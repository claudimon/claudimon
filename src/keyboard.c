#include "keyboard.h"
#include "vga.h"

/* ============================================
   PS/2 KEYBOARD SCANCODE MAP
   
   When a key is pressed, the keyboard sends a
   "scancode" — a number identifying the key.
   This table maps scancode → ASCII character.
   0 means "no printable character" (shift, ctrl etc.)
   ============================================ */

static const char scancode_map[128] = {
    0,   0,  '1','2','3','4','5','6','7','8','9','0','-','=', 0,  '\t',
   'q','w','e','r','t','y','u','i','o','p','[',']','\n', 0,
   'a','s','d','f','g','h','j','k','l',';','\'','`', 0, '\\',
   'z','x','c','v','b','n','m',',','.','/', 0, '*', 0, ' ',
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

/* Small circular buffer to hold keypresses */
#define KEY_BUFFER_SIZE 256
static char key_buffer[KEY_BUFFER_SIZE];
static volatile int buf_head = 0;
static volatile int buf_tail = 0;

/* ---- Read a byte from an I/O port ---- */
static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

/* ---- Write a byte to an I/O port ---- */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

/* ---- Send End-Of-Interrupt to the PIC ---- */
static void pic_send_eoi(void) {
    outb(0x20, 0x20);  /* 0x20 = PIC1 command port, 0x20 = EOI command */
}

/* ---- Called when IRQ1 fires (key pressed) ---- */
void keyboard_handler(void) {
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);

    /* Ignore key-release events (bit 7 set) */
    if (scancode & 0x80) {
        pic_send_eoi();
        return;
    }

    char c = scancode_map[scancode];
    if (c != 0) {
        /* Store in ring buffer */
        int next = (buf_head + 1) % KEY_BUFFER_SIZE;
        if (next != buf_tail) {   /* Don't overflow */
            key_buffer[buf_head] = c;
            buf_head = next;
        }
    }

    pic_send_eoi();
}

/* ---- Block until a key is available, then return it ---- */
char keyboard_getchar(void) {
    while (buf_head == buf_tail) {
        __asm__ volatile ("hlt"); /* Sleep until next interrupt */
    }
    char c = key_buffer[buf_tail];
    buf_tail = (buf_tail + 1) % KEY_BUFFER_SIZE;
    return c;
}

/* ============================================
   IDT & PIC SETUP
   
   To receive keyboard interrupts we need to:
   1. Reprogram the PIC so IRQ1 → interrupt 0x21
   2. Set up an IDT entry pointing to our handler
   3. Enable interrupts (sti)
   ============================================ */

/* IDT entry: 8 bytes describing one interrupt handler */
struct idt_entry {
    uint16_t offset_low;   /* Lower 16 bits of handler address */
    uint16_t selector;     /* Code segment selector (0x08 = kernel code) */
    uint8_t  zero;         /* Always 0 */
    uint8_t  type_attr;    /* Type + attributes (0x8E = interrupt gate) */
    uint16_t offset_high;  /* Upper 16 bits of handler address */
} __attribute__((packed));

/* IDT pointer passed to the lidt instruction */
struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

#define IDT_SIZE 256
static struct idt_entry idt[IDT_SIZE];
static struct idt_ptr   idtp;

/* Assembly stub declared in keyboard_asm.asm */
extern void keyboard_isr(void);

static void idt_set_gate(uint8_t num, uint32_t handler) {
    idt[num].offset_low  = handler & 0xFFFF;
    idt[num].selector    = 0x08;          /* Kernel code segment */
    idt[num].zero        = 0;
    idt[num].type_attr   = 0x8E;          /* Present, ring 0, 32-bit interrupt gate */
    idt[num].offset_high = (handler >> 16) & 0xFFFF;
}

/* Reprogram the 8259 PIC so IRQs don't clash with CPU exceptions */
static void pic_remap(void) {
    /* Start initialisation sequence */
    outb(0x20, 0x11);   /* PIC1 command */
    outb(0xA0, 0x11);   /* PIC2 command */
    /* Set vector offsets */
    outb(0x21, 0x20);   /* PIC1 vectors start at 0x20 */
    outb(0xA1, 0x28);   /* PIC2 vectors start at 0x28 */
    /* Set up cascading */
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    /* 8086 mode */
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    /* Mask all IRQs except IRQ1 (keyboard) */
    outb(0x21, 0xFD);   /* 0xFD = 11111101 — only IRQ1 unmasked */
    outb(0xA1, 0xFF);   /* Mask all PIC2 IRQs */
}

void keyboard_init(void) {
    pic_remap();

    /* Install keyboard ISR at interrupt 0x21 (PIC1 offset 0x20 + IRQ1) */
    idt_set_gate(0x21, (uint32_t)keyboard_isr);

    /* Load the IDT */
    idtp.limit = (sizeof(struct idt_entry) * IDT_SIZE) - 1;
    idtp.base  = (uint32_t)&idt;
    __asm__ volatile ("lidt (%0)" : : "r"(&idtp));

    /* Enable interrupts! */
    __asm__ volatile ("sti");
}
