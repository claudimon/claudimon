#include "vga.h"
#include "keyboard.h"
#include "shell.h"

void kernel_main(void) {
    /* Initialise the screen */
    terminal_init();

    /* Initialise keyboard (sets up IDT + PIC + enables interrupts) */
    keyboard_init();

    /* Hand off to the shell — never returns */
    shell_run();
}
