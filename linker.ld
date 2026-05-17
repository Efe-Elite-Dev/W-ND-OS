ENTRY(_start)
OUTPUT_FORMAT(elf32-i386)

SECTIONS
{
    . = 0x100000; /* Çekirdek 1MB noktasına yüklenecek */

    .text : ALIGN(4K)
    {
        *(.multiboot) /* Başlık en başta olmak zorunda */
        *(.text)
    }

    .rodata : ALIGN(4K)
    {
        *(.rodata*)
    }

    .data : ALIGN(4K)
    {
        *(.data)
    }

    .bss : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
    }
}
