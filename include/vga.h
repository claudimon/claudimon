#ifndef VGA_H
#define VGA_H

#include <stdint.h>
#include <stddef.h>

/* ============================================
   VGA TEXT MODE DRIVER
   
   Your screen in text mode is a grid of 80x25 characters.
   Each character takes up 2 bytes in memory:
     Byte 1: The ASCII character to display
     Byte 2: The colour (foreground + background)
   
   VGA text buffer starts at memory address 0xB8000
   ============================================ */

/* VGA Colours - these are the 16 standard colours */
typedef enum {
    VGA_COLOR_BLACK         = 0,
    VGA_COLOR_BLUE          = 1,
    VGA_COLOR_GREEN         = 2,
    VGA_COLOR_CYAN          = 3,
    VGA_COLOR_RED           = 4,
    VGA_COLOR_MAGENTA       = 5,
    VGA_COLOR_BROWN         = 6,
    VGA_COLOR_LIGHT_GREY    = 7,
    VGA_COLOR_DARK_GREY     = 8,
    VGA_COLOR_LIGHT_BLUE    = 9,
    VGA_COLOR_LIGHT_GREEN   = 10,
    VGA_COLOR_LIGHT_CYAN    = 11,
    VGA_COLOR_LIGHT_RED     = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN   = 14,  /* Actually yellow */
    VGA_COLOR_WHITE         = 15,
} vga_color_t;

/* Screen dimensions */
#define VGA_WIDTH  80
#define VGA_HEIGHT 25

/* Pack foreground + background colour into 1 byte */
static inline uint8_t vga_entry_color(vga_color_t fg, vga_color_t bg) {
    return fg | (bg << 4);
}

/* Pack character + colour into 1 16-bit VGA entry */
static inline uint16_t vga_entry(unsigned char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

/* Public terminal functions */
void terminal_init(void);
void terminal_putchar(char c);
void terminal_write(const char* str);
void terminal_set_color(vga_color_t fg, vga_color_t bg);

#endif /* VGA_H */
