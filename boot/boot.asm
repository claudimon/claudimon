section .multiboot_header
align 8

header_start:
    dd 0xe85250d6
    dd 0
    dd header_end - header_start
    dd 0x100000000 - (0xe85250d6 + 0 + (header_end - header_start))
    dw 0
    dw 0
    dd 8
header_end:

section .bss
align 16
stack_bottom:
    resb 16384
stack_top:

section .text
bits 32
global _start

_start:
    mov esp, stack_top

    ; eax = multiboot2 magic (0x36d76289)
    ; ebx = pointer to multiboot2 info structure
    ; Pass both to kernel_main(magic, mb_addr)
    push ebx        ; arg2: multiboot info pointer
    push eax        ; arg1: magic number

    extern kernel_main
    call kernel_main

.hang:
    cli
    hlt
    jmp .hang
