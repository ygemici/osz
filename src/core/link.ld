FBUF_ADDRESS = 0xffffffffe0000000;
CORE_ADDRESS = 0xffffffffffe00000;
ENTRY(_start)
OUTPUT_FORMAT(elf64-x86-64)
SECTIONS
{
    . = FBUF_ADDRESS;
        fb = .;
    /* make sure it starts with bootboot */
    . = CORE_ADDRESS;
        bootboot = .;
        . += 4096;
        environment = .;
        . += 4096;

    .text : AT(ADDR(.text) - CORE_ADDRESS)
    {
        _code = .;
        *(.text)
        . = ALIGN(4096);
        _data = .;
        *(.data)
        . = ALIGN(4096);
    }
/*
   .data : AT(ADDR(.data) - CORE_ADDRESS)
   {
        _data = .;
        *(.data)
        . = ALIGN(4096);
   }
*/
   _end = .;

   /DISCARD/ :
   {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
   }
}
