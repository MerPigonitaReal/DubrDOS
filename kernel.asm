;; kernel.asm
bits 32         ; nasm directive - 32 bit
global entry
extern _kmain   ; kmain is defined in the c file

section .text
entry:  
    jmp start

    ; multiboot spec
    align 4
    dd 0x1BADB002            ; magic
    dd 0x00                  ; flags
    dd -(0x1BADB002 + 0x00)  ; checksum. m+f+c should be zero

start:
    cli                     ; block interrupts
    mov esp, stack_space    ; set stack pointer
    call _kmain
    hlt                     ; halt the CPU

section .bss
resb 8192                 ; 8KB for stack
stack_space:
