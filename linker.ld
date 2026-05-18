ENTRY(_start)

SECTIONS
{
    /* Çekirdeği 1MB noktasına mühürlüyoruz (Standart Multiboot) */
    . = 1M;

    .text BLOCK(4K) : ALIGN(4K)
    {
        *(.multiboot) /* Önce multiboot header gelmek ZORUNDA */
        *(.text)
    }

    .rodata BLOCK(4K) : ALIGN(4K)
    {
        *(.rodata)
    }

    .data BLOCK(4K) : ALIGN(4K)
    {
        *(.data)
    }

    .bss BLOCK(4K) : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
    }
}
