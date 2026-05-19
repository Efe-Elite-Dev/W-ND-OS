bits 32                         ; 32-bit modda çalışacağımızı belirtiyoruz
section .multiboot
    align 4
    dd 0x1BADB002               ; Multiboot sihirli numarası (GRUB bunu arar)
    dd 0x00                     ; Flaglar (Şimdilik grafik modu vs. istemiyoruz, düz VGA)
    dd - (0x1BADB002 + 0x00)    ; Checksum (GRUB doğrulaması için şart)

section .text
global _start
extern kernel_main              ; kernel.c içindeki ana fonksiyonumuz

_start:
    cli                         ; Kesmeleri (Interrupts) kapat
    mov esp, stack_space        ; Stack pointer'ı ayarla
    call kernel_main            ; C kodumuza zıpla
    
halt_loop:
    hlt                         ; Eğer kernel_main'den dönerse işlemciyi uyut
    jmp halt_loop

section .bss
resb 8192                       ; 8KB'lık stack alanı ayır
stack_space:

; GitHub Actions'taki linker uyarısını çözen sihirli satır:
section .note.GNU-stack noalloc noexec nowrite progbits
