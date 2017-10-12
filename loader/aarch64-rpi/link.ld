SECTIONS
{
    . = 0x80000;
    .text : { KEEP(*(.text.boot)) *(.text .text.* .gnu.linkonce.t*) }
    .rodata : { *(.rodata .rodata.* .gnu.linkonce.r*) }
    .data : { *(.data .data.* .gnu.linkonce.d*) }
    .bss (NOLOAD) : {
        __bss_start = ALIGN(16);
        *(.bss .bss.*)
        *(COMMON)
        /* buffers with alignment requirements */
        __mbr = ALIGN(16);
        . += 512;
        __mailbox = ALIGN(16);
        . += 256;
        . = ALIGN(4096);
        __bootboot = .;
        . += 4096;
        __environment = .;
        . += 4096;
        __bss_end = .;
    }
    _end = .;
}
__bss_size = (__bss_end - __bss_start)>>3;
