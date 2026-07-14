; =========================================================
; Wind OS / Sky-Core-OS  -  boot.asm
; Multiboot 1 + VBE Linear Framebuffer (1024x768x32)
; =========================================================

MBOOT_MAGIC    equ 0x1BADB002
MBOOT_FLAGS    equ 0x00000007   ; bit0=hizala  bit1=bellek  bit2=video
MBOOT_CHECKSUM equ -(MBOOT_MAGIC + MBOOT_FLAGS)

section .multiboot
align 4
    dd MBOOT_MAGIC
    dd MBOOT_FLAGS
    dd MBOOT_CHECKSUM
    ; ---- Adres alanları: bit16=0 olduğunda 0 yazılır ----
    dd 0        ; header_addr   (kullanılmıyor)
    dd 0        ; load_addr     (kullanılmıyor)
    dd 0        ; load_end_addr (kullanılmıyor)
    dd 0        ; bss_end_addr  (kullanılmıyor)
    dd 0        ; entry_addr    (kullanılmıyor)
    ; ---- Video modu isteği ----
    dd 0        ; mode_type  0 = linear grafik
    dd 1024     ; genişlik
    dd 768      ; yükseklik
    dd 32       ; bit/piksel (32-bit Kuantum Kalite)

section .bss
align 16
stack_bottom:
    resb 16384      ; 16 KB kernel stack
stack_top:

section .text
global _start
extern kernel_main  ; kernel.c'deki void kernel_main(multiboot_info_t*)

_start:
    cli                 ; kesmeler kapalı
    mov  esp, stack_top ; stack kur
    push ebx            ; DONANIM BİLGİSİNİ C'YE GÖNDER (mbi parametresi)
    call kernel_main

.halt:
    cli
    hlt
    jmp .halt
