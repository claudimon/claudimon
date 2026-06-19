#include "vga.h"
#include "keyboard.h"
#include "fs.h"
#include "shell.h"
#include "stdint.h"

/* Exposed to shell.c for the mem command */
uint32_t total_memory_kb = 0;

/* Multiboot2 info structure (partial) */
typedef struct {
    uint32_t total_size;
    uint32_t reserved;
} mb2_info_t;

typedef struct {
    uint32_t type;
    uint32_t size;
} mb2_tag_t;

typedef struct {
    uint32_t type;      /* 4 = basic memory info */
    uint32_t size;
    uint32_t mem_lower; /* KB below 1MB */
    uint32_t mem_upper; /* KB above 1MB */
} mb2_tag_memory_t;

/* boot.asm passes multiboot info pointer in ebx */
void kernel_main(uint32_t magic, uint32_t mb_addr) {
    terminal_init();

    /* Parse multiboot2 info to get memory size */
    if (magic == 0x36d76289 && mb_addr != 0) {
        mb2_tag_t* tag = (mb2_tag_t*)(mb_addr + 8);
        mb2_info_t* info = (mb2_info_t*)mb_addr;
        uint32_t end = mb_addr + info->total_size;

        while ((uint32_t)tag < end && tag->type != 0) {
            if (tag->type == 4) {
                mb2_tag_memory_t* mem = (mb2_tag_memory_t*)tag;
                total_memory_kb = mem->mem_lower + mem->mem_upper;
                break;
            }
            /* Tags are 8-byte aligned */
            uint32_t next = (uint32_t)tag + tag->size;
            next = (next + 7) & ~7;
            tag = (mb2_tag_t*)next;
        }
    }

    /* Fallback: QEMU default is 32MB */
    if (total_memory_kb == 0) total_memory_kb = 32768;

    keyboard_init();
    fs_init();
    shell_run();
}
