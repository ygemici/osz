SECTIONS
{
    . = 0x80000;
    .text : { KEEP(*(.text.boot)) *(.text .text.* .gnu.linkonce.t*) }
    .rodata : { *(.rodata .rodata.* .gnu.linkonce.r*) }
    _etext = .;
    .data : { *(.data .data.* .gnu.linkonce.d*) }
    .bss (NOLOAD) : {
        __bss_start = ALIGN(16);
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = ALIGN(16);
    }
    . = ALIGN(4096);
    _end = .;
}
__bss_size = (__bss_end - __bss_start)>>3;
