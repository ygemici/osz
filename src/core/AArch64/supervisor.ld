/* user space addresses */
TCB_ADDRESS   = 0;
MQ_ADDRESS    = 4096;
TEXT_ADDRESS  = 0x0000000000200000;
BSS_ADDRESS   = 0x0000000100000000;
STACK_ADDRESS = 0x0000800000000000;
/* kernel space addresses */
FBUF_ADDRESS  = 0xffffffffe0000000;
CORE_ADDRESS  = 0xffffffffffe00000;

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
        /* code */
        PROVIDE(_code = .);
        *(.text)
        . = ALIGN(4096);
        /* initialized and preallocated data */
        PROVIDE(_data = .);
        *(.rodata)
        *(.data)
        . = ALIGN(4096);
        /* uninitialized data */
        PROVIDE(__bss_start = .);
        /* mappings hack */
        tmpctrl = .;
        . += 4096;
        tmpmap = .;
        . += 4096;
        tmppte = .;
        . += 4096;
        /* CPU Control Block. We need static address for that */
        ccb = .;
        . += 4096;
        /* physical memory manager free memory entries */
        pmm_entries = .;
    }
   PROVIDE(_etext = .);

   /DISCARD/ :
   {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
        *(.gnu.*)
   }
}
