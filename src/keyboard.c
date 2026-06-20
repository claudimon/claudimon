#include "keyboard.h"
#include "vga.h"

/* ============================================
   KEYBOARD DRIVER - POLLING MODE (UK QWERTY)

   Returns:
     normal ASCII for printable keys
     '\n' for Enter
     '\b' for Backspace
     '\x1b' followed by '[' 'A'/'B'/'C'/'D' for arrows
     0x01 = Ctrl+S (save), 0x11 = Ctrl+Q (quit) -- see editor.c
   ============================================ */

/* UK QWERTY scancode map (unshifted) */
static const char scancode_map[128] = {
    0,   0,  '1','2','3','4','5','6','7','8','9','0','-','=', 0,  '\t',
   'q','w','e','r','t','y','u','i','o','p','[',']','\n', 0,
   'a','s','d','f','g','h','j','k','l',';','\'','#', 0, '\\',
   'z','x','c','v','b','n','m',',','.','/', 0, '*', 0, ' ',
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

/* UK QWERTY scancode map (shifted) */
static const char scancode_map_shift[128] = {
    0,   0,  '!','"','#','$','%','^','&','*','(',')','_','+', 0,  '\t',
   'Q','W','E','R','T','Y','U','I','O','P','{','}','\n', 0,
   'A','S','D','F','G','H','J','K','L',':','@','~', 0, '|',
   'Z','X','C','V','B','N','M','<','>','?', 0, '*', 0, ' ',
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

#define SC_LSHIFT   0x2A
#define SC_RSHIFT   0x36
#define SC_LCTRL    0x1D
#define SC_BACKSPACE 0x0E
#define SC_UP       0x48
#define SC_DOWN     0x50
#define SC_LEFT     0x4B
#define SC_RIGHT    0x4D

static int shift_held = 0;
static int ctrl_held  = 0;

/* Buffered "pending" bytes for multi-byte sequences (arrow keys) */
static char pending[4];
static int  pending_len = 0;
static int  pending_pos = 0;

static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

void keyboard_init(void) {
    /* Nothing needed for polling mode */
}

/* Returns the raw key event, handling modifier keys internally.
   Returns 0 if nothing printable happened (caller should loop). */
static char read_raw_key(void) {
    uint8_t scancode;
    static uint8_t last_make = 0;

    while (1) {
        while (!(inb(0x64) & 0x01));
        scancode = inb(0x60);

        uint8_t released = scancode & 0x80;
        uint8_t code      = scancode & 0x7F;

        if (code == SC_LSHIFT || code == SC_RSHIFT) {
            shift_held = !released;
            continue;
        }
        if (code == SC_LCTRL) {
            ctrl_held = !released;
            continue;
        }

        if (released) { last_make = 0; continue; }
        if (scancode == last_make) continue;  /* ignore key-repeat duplicates from re-poll */
        last_make = scancode;

        /* Arrow keys -> emit an escape sequence the shell understands */
        if (code == SC_UP)    { pending[0]='['; pending[1]='A'; pending_len=2; pending_pos=0; return '\x1b'; }
        if (code == SC_DOWN)  { pending[0]='['; pending[1]='B'; pending_len=2; pending_pos=0; return '\x1b'; }
        if (code == SC_RIGHT) { pending[0]='['; pending[1]='C'; pending_len=2; pending_pos=0; return '\x1b'; }
        if (code == SC_LEFT)  { pending[0]='['; pending[1]='D'; pending_len=2; pending_pos=0; return '\x1b'; }

        if (code == SC_BACKSPACE) return '\b';

        /* Ctrl combos for the editor: Ctrl+S = save, Ctrl+Q = quit */
        if (ctrl_held) {
            char base = shift_held ? scancode_map_shift[code] : scancode_map[code];
            if (base == 's' || base == 'S') return 0x13;  /* DC3 = Ctrl+S */
            if (base == 'q' || base == 'Q') return 0x11;  /* DC1 = Ctrl+Q */
            continue;
        }

        char c = shift_held ? scancode_map_shift[code] : scancode_map[code];
        if (c != 0) return c;
    }
}

char keyboard_getchar(void) {
    if (pending_pos < pending_len) {
        return pending[pending_pos++];
    }
    return read_raw_key();
}
