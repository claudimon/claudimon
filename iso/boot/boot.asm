; boot.asm - The very first code that runs when our kernel is loaded
; We use Multiboot2 so GRUB knows how to load us

; =============================================
; MULTIBOOT2 HEADER
; This is a magic "signature" GRUB looks for
; to know this is a valid kernel
; =============================================

section .multiboot_header
align 8

header_start:
    dd 0xe85250d6           ; Multiboot2 magic number (GRUB looks for this)
    dd 0                    ; Architecture: 0 = i386/x86 protected mode
    dd header_end - header_start  ; Total header length
    ; Checksum: makes all 4 values add up to 0 (mod 2^32)
    dd 0x100000000 - (0xe85250d6 + 0 + (header_end - header_start))

    ; End tag - tells GRUB "that's all the header info"
    dw 0    ; type = 0 (end tag)
    dw 0    ; flags
    dd 8    ; size
header_end:

; =============================================
; BSS SECTION - Uninitialized data (our stack)
; =============================================
section .bss
align 16
stack_bottom:
    resb 16384      ; Reserve 16 KB for our stack
stack_top:

; =============================================
; TEXT SECTION - Our actual code
; =============================================
section .text
bits 32             ; GRUB puts us in 32-bit protected mode first
global _start       ; Make _start visible to the linker

_start:
    ; Set up our stack
    ; The stack grows downward, so we point to the top
    mov esp, stack_top

    ; Call our C kernel main function
    extern kernel_main
    call kernel_main

    ; If kernel_main ever returns (it shouldn't), hang the CPU
.hang:
    cli             ; Disable interrupts
    hlt             ; Halt the CPU
    jmp .hang       ; Loop forever just in case
