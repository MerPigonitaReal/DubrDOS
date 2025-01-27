/*
*  link.ld
*/
OUTPUT_FORMAT(pei-i386)    
ENTRY(entry)
SECTIONS
{
    . = 0x00100000;

    .text : {
        *(.multiboot)
        *(.text)
    }

    .data : {
        *(.data)
    }

    .bss : {
        *(.bss)
        *(COMMON)
    }
}