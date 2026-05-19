section .multiboot
align 4
    MULTIBOOT_MAGIC    equ 0x1BADB002
    MULTIBOOT_FLAGS    equ 0x00000007 ; Bit 0 (hizala) + Bit 1 (mem info) + Bit 2 (video)
    MULTIBOOT_CHECKSUM equ -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

    ; 1. Magic (Offset 0)
    dd MULTIBOOT_MAGIC
    ; 2. Flags (Offset 4)
    dd MULTIBOOT_FLAGS
    ; 3. Checksum (Offset 8)
    dd MULTIBOOT_CHECKSUM

    ; --- ZORUNLU PADDING (AOUT KLUDGE ALANLARI) ---
    ; Bit 16 kapalı olsa bile GRUB'ın 32. byte'a kadar kaymaması için bu 5 alan SIFIR olmalıdır!
    dd 0    ; header_addr   (Offset 12)
    dd 0    ; load_addr     (Offset 16)
    dd 0    ; load_end_addr (Offset 20)
    dd 0    ; bss_end_addr  (Offset 24)
    dd 0    ; entry_addr    (Offset 28)

    ; --- GRAFİK MODU ALANI (Tam olarak Offset 32'de olmak zorunda) ---
    dd 0    ; mode_type (0 = Lineer VBE Modu)
    dd 1024 ; width
    dd 768  ; height
    dd 32   ; depth (bpp)

; Buradan sonra normal kernel girişin başlayabilir
global _start
_start:
    cli
    ; ...
