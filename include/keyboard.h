#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

/* ============================================
   KEYBOARD DRIVER
   
   The keyboard communicates via two I/O ports:
   - 0x60: Data port (read the key scancode)
   - 0x64: Status port (check if data is ready)
   
   When a key is pressed, the keyboard controller
   triggers IRQ1, which maps to interrupt 0x21.
   We read port 0x60 to find out which key.
   ============================================ */

#define KEYBOARD_DATA_PORT   0x60
#define KEYBOARD_STATUS_PORT 0x64

/* Initialise keyboard interrupt handling */
void keyboard_init(void);

/* Called by the interrupt handler when a key is pressed */
void keyboard_handler(void);

/* Read one character (blocks until a key is pressed) */
char keyboard_getchar(void);

#endif /* KEYBOARD_H */
