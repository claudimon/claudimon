#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "stdint.h"

#define KEYBOARD_DATA_PORT   0x60
#define KEYBOARD_STATUS_PORT 0x64

/* Available keyboard layouts */
typedef enum {
    LAYOUT_UK_QWERTY = 0,
    LAYOUT_US_QWERTY,
    LAYOUT_DVORAK,
    LAYOUT_COLEMAK,
    LAYOUT_SWEDISH,
    LAYOUT_GERMAN,
    LAYOUT_FRENCH,
    LAYOUT_COUNT
} keyboard_layout_t;

void keyboard_init(void);
char keyboard_getchar(void);

/* Layout switching */
void        keyboard_set_layout(keyboard_layout_t layout);
keyboard_layout_t keyboard_get_layout(void);
const char* keyboard_layout_name(keyboard_layout_t layout);

/* Internal: implemented in keyboard_layouts.c */
const char* keyboard_layout_table_name(int layout);
char        keyboard_layout_lookup(int layout, uint8_t scancode, int shifted);

#endif
