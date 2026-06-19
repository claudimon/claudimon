#include "vga.h"
#include "keyboard.h"
#include "fs.h"
#include "shell.h"

void kernel_main(void) {
    terminal_init();
    keyboard_init();
    fs_init();
    shell_run();
}
