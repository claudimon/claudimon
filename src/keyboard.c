#include "keyboard.h"
#include "vga.h"

/* ============================================
   KEYBOARD DRIVER - POLLING MODE
   Now supports multiple selectable layouts.
   ============================================ */

#define SC_LSHIFT    0x2A
#define SC_RSHIFT    0x36
#define SC_LCTRL     0x1D
#define SC_BACKSPACE 0x0E
#define SC_UP        0x48
#define SC_DOWN      0x50
#define SC_LEFT      0x4B
#define SC_RIGHT     0x4D

static int shift_held = 0;
static int ctrl_held  = 0;
static keyboard_layout_t current_layout = LAYOUT_UK_QWERTY;

static char pending[4];
static int  pending_len = 0;
static int  pending_pos = 0;

static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

void keyboard_init(void) {
    current_layout = LAYOUT_UK_QWERTY;
}

void keyboard_set_layout(keyboard_layout_t layout) {
    if (layout >= 0 && layout < LAYOUT_COUNT) current_layout = layout;
}

keyboard_layout_t keyboard_get_layout(void) {
    return current_layout;
}

const char* keyboard_layout_name(keyboard_layout_t layout) {
    return keyboard_layout_table_name(layout);
}

static char read_raw_key(void) {
    uint8_t scancode;
    static uint8_t last_make = 0;

    while (1) {
        while (!(inb(0x64) & 0x01));
        scancode = inb(0x60);

        uint8_t released = scancode & 0x80;
        uint8_t code      = scancode & 0x7F;

        if (code == SC_LSHIFT || code == SC_RSHIFT) { shift_held = !released; continue; }
        if (code == SC_LCTRL)                       { ctrl_held  = !released; continue; }

        if (released) { last_make = 0; continue; }
        if (scancode == last_make) continue;
        last_make = scancode;

        if (code == SC_UP)    { pending[0]='['; pending[1]='A'; pending_len=2; pending_pos=0; return '\x1b'; }
        if (code == SC_DOWN)  { pending[0]='['; pending[1]='B'; pending_len=2; pending_pos=0; return '\x1b'; }
        if (code == SC_RIGHT) { pending[0]='['; pending[1]='C'; pending_len=2; pending_pos=0; return '\x1b'; }
        if (code == SC_LEFT)  { pending[0]='['; pending[1]='D'; pending_len=2; pending_pos=0; return '\x1b'; }

        if (code == SC_BACKSPACE) return '\b';

        if (ctrl_held) {
            char base = keyboard_layout_lookup(current_layout, code, shift_held);
            if (base == 's' || base == 'S') return 0x13;
            if (base == 'q' || base == 'Q') return 0x11;
            continue;
        }

        char c = keyboard_layout_lookup(current_layout, code, shift_held);
        if (c != 0) return c;
    }
}

char keyboard_getchar(void) {
    if (pending_pos < pending_len) return pending[pending_pos++];
    return read_raw_key();
}
