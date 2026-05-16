ENTRY(start)

SECTIONS
{
    /* Çekirdek 1 MB (1 Megabyte) adresinden başlar */
    . = 1M;

    /* Multiboot başlığı en başta olmalı ki GRUB tanısın */
    .text BLOCK(4K) : ALIGN(4K)
    {
        *(.multiboot)
        *(.text)
    }

    /* Salt okunur veriler */
    .rodata BLOCK(4K) : ALIGN(4K)
    {
        *(.rodata)
    }

    /* Başlangıçta değer atanmış veriler */
    .data BLOCK(4K) : ALIGN(4K)
    {
        *(.data)
    }

    /* Başlangıçta sıfır olan veriler (BSS) */
    .bss BLOCK(4K) : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
    }
}ENTRY(start)

SECTIONS
{
    /* Çekirdek 1 MB (1 Megabyte) adresinden başlar */
    . = 1M;

    /* Multiboot başlığı en başta olmalı ki GRUB tanısın */
    .text BLOCK(4K) : ALIGN(4K)
    {
        *(.multiboot)
        *(.text)
    }

    /* Salt okunur veriler */
    .rodata BLOCK(4K) : ALIGN(4K)
    {
        *(.rodata)
    }

    /* Başlangıçta değer atanmış veriler */
    .data BLOCK(4K) : ALIGN(4K)
    {
        *(.data)
    }

    /* Başlangıçta sıfır olan veriler (BSS) */
    .bss BLOCK(4K) : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
    }
}
