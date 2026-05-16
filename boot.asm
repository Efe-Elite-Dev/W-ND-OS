; Wind OS Bootloader - Sky OS Engine Compatible
MBOOT_PAGE_ALIGN    equ 1 << 0
MBOOT_MEMORY_INFO   equ 1 << 1
MBOOT_GRAPHICS_MODE equ 1 << 2
MBOOT_FLAGS         equ MBOOT_PAGE_ALIGN | MBOOT_MEMORY_INFO | MBOOT_GRAPHICS_MODE
MBOOT_MAGIC         equ 0x1BADB002
MBOOT_CHECKSUM      equ -(MBOOT_MAGIC + MBOOT_FLAGS)

section .multiboot
align 4
    dd MBOOT_MAGIC
    dd MBOOT_FLAGS
    dd MBOOT_CHECKSUM
    
    ; VBE Grafik Modu Gereksinimleri (800x600x32)
    dd 0
    dd 0
    dd 0
    dd 0
    dd 0
    dd 800
    dd 600
    dd 32

section .text
global start
extern kernel_main

start:
    cli                         ; Kesmeleri kapat
    mov esp, stack_space        ; Stack göstergesini ayarla
    
    push ebx                    ; Multiboot info yapısının adresini stack'e it (mboot parametresi)
    call kernel_main            ; C çekirdeğine atla
    
.halt:
    hlt
    jmp .halt

section .bss
align 16
stack_space:
    resb 8192                   ; 8KB Güvenli Stack Alanı
