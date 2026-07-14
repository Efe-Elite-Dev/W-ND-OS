ENTRY(_start)
SECTIONS {
    . = 1M;
    
    .text BLOCK(4K) : ALIGN(4K) {
        *(.multiboot)
        *(.text .text.*)
    }
    
    /* İŞTE HAYAT KURTARAN KISIM: .rodata.* ile harflerin silinmesi engellendi! */
    .rodata BLOCK(4K) : ALIGN(4K) { 
        *(.rodata .rodata.*) 
    }
    
    .data BLOCK(4K) : ALIGN(4K) { 
        *(.data .data.*)   
    }
    
    .bss BLOCK(4K) : ALIGN(4K) { 
        *(COMMON) 
        *(.bss .bss.*) 
    }
}
