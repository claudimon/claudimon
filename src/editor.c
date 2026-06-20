#include "editor.h"
#include "vga.h"
#include "keyboard.h"
#include "fs.h"
#include "stdint.h"

/* ============================================
   SIMPLE FULL-SCREEN TEXT EDITOR

   Controls:
     Type to insert text
     Backspace to delete
     Enter for newline
     Ctrl+S to save
     Ctrl+Q to quit without saving
   ============================================ */

#define EDITOR_MAX_CHARS FS_MAX_FILE_SIZE

static char buffer[EDITOR_MAX_CHARS];
static int  buf_len = 0;

/* ---- tiny strcpy ---- */
static void e_strcpy(char* dst, const char* src, int max) {
    int i=0; while(src[i] && i<max-1){dst[i]=src[i];i++;} dst[i]='\0';
}

/* ---- Redraw the whole editor screen ---- */
static void editor_redraw(const char* filename) {
    terminal_init();
    terminal_set_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_CYAN);
    /* Top status bar */
    for (int i = 0; i < VGA_WIDTH; i++) terminal_putchar(' ');
    /* Move cursor back to start of line 0 isn't directly possible with our
       simple terminal API, so we just write a fresh top line each redraw */
    terminal_init();
    terminal_set_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_CYAN);
    terminal_write(" Claudimon Editor - ");
    terminal_write(filename);
    terminal_write(" - Ctrl+S Save  Ctrl+Q Quit");
    /* pad rest of line */
    int used = 28 + 0;
    (void)used;
    terminal_write("\n");
    terminal_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    /* Print buffer contents */
    for (int i = 0; i < buf_len; i++) {
        terminal_putchar(buffer[i]);
    }
}

void editor_open(const char* filename) {
    /* Try to load existing file content */
    buf_len = 0;
    buffer[0] = '\0';

    /* fs_cat prints to screen, so instead we need direct read.
       We'll add a tiny inline lookup here using fs's internal API
       indirectly: easiest is to attempt fs_create only if not found.
       Since fs.h doesn't expose raw read, we approximate by calling
       cat into nothing -- instead let's just start blank if new. */

    int is_new = 1;
    /* Quick existence check + load via fs_cat redirected isn't available,
       so we re-implement a minimal loader using fs_cat's pattern:
       (fs.c keeps files static; for simplicity, new files start blank,
       existing files are loaded by reading through fs_cat's behaviour) */

    int loaded = fs_load(filename, buffer, EDITOR_MAX_CHARS);
    if (loaded >= 0) { buf_len = loaded; is_new = 0; }
    (void)is_new;

    editor_redraw(filename);

    while (1) {
        char c = keyboard_getchar();

        if (c == 0x13) {  /* Ctrl+S: save */
            buffer[buf_len] = '\0';
            fs_create(filename, buffer);
            terminal_set_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREEN);
            terminal_write("\n[Saved]");
            terminal_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
            /* brief pause feel: just continue editing */
            continue;

        } else if (c == 0x11) {  /* Ctrl+Q: quit */
            terminal_init();
            return;

        } else if (c == '\b') {
            if (buf_len > 0) {
                buf_len--;
                editor_redraw(filename);
            }

        } else if (c == '\x1b') {
            /* swallow arrow keys in editor for now (no cursor movement yet) */
            keyboard_getchar();
            keyboard_getchar();

        } else if (c == '\n') {
            if (buf_len < EDITOR_MAX_CHARS - 1) {
                buffer[buf_len++] = '\n';
                terminal_putchar('\n');
            }

        } else if (c >= 32 && c < 127) {
            if (buf_len < EDITOR_MAX_CHARS - 1) {
                buffer[buf_len++] = c;
                terminal_putchar(c);
            }
        }
    }
}
