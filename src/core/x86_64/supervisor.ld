/* kernel space addresses */
FBUF_ADDRESS  = 0xffffffffe0000000;
DSTMQ_ADDRESS = 0xffffffffffc00000;
CORE_ADDRESS  = 0xffffffffffe00000;

ENTRY(_start)
OUTPUT_FORMAT(elf64-x86-64)
SECTIONS
{
    . = FBUF_ADDRESS;
        fb = .;
	/* destination thread's message queue mapped, must match ksend() */
	. = DSTMQ_ADDRESS;
        . += 4096;
		tmpmq = .;
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
        *(.bss)
        . = ALIGN(4096);
        /* user mode code */
        PROVIDE(_usercode = .);
        *(.text.user0)
        *(.text.user)
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
        tmppde = .;
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
