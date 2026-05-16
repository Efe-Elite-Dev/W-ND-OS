ENTRY(_start)

SECTIONS
{
    /* Çekirdeği 1MB (0x100000) adresine yerleştiriyoruz */
    . = 0x100000;

    .text BLOCK(4K) : ALIGN(4K)
    {
        KEEP(*(.multiboot)) /* KRİTİK: GRUB bu başlığı ilk sırada bulmak ZORUNDA! */
        *(.text)
    }

    .rodata BLOCK(4K) : ALIGN(4K) { *(.rodata*) }
    .data BLOCK(4K)   : ALIGN(4K) { *(.data)   }
    .bss BLOCK(4K)    : ALIGN(4K) { *(.bss)    }
}
