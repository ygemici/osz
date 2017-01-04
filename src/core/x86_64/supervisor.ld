/* kernel space addresses */
FBUF_ADDRESS  = 0xffffffffe0000000;
TMPQ_ADDRESS    = 0xffffffffffc00000;
CORE_ADDRESS  = 0xffffffffffe00000;

ENTRY(_start)
OUTPUT_FORMAT(elf64-x86-64)
PHDRS
{
  text PT_LOAD FILEHDR PHDRS ;
}
SECTIONS
{
    /* framebuffer address */
    . = FBUF_ADDRESS;
        fb = .;
    /* temporary message queue address */
    . = TMPQ_ADDRESS;
        tmpmq = .;
    /* make sure it starts with bootboot */
    . = CORE_ADDRESS;
        bootboot = .;
        . += 4096;
        environment = .;
        . += 4096;

    /* put headers in load segment so that we won't have to copy
        text segment on loading as it will be aligned */
    .text CORE_ADDRESS + SIZEOF_HEADERS + (8192): AT(ADDR(.text) - CORE_ADDRESS + SIZEOF_HEADERS)
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
        *(.text.user)
    } :text
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
    tmpmqctrl = .;
    . += 4096;
    tmp2map = .;
    . += 4096;
    tmpalarm = .;
    . += 4096;
    /* CPU Control Block. We need static address for that */
    ccb = .;
    . += 4096;
    /* physical memory manager free memory entries */
    pmm_entries = .;

   /DISCARD/ :
   {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
        *(.gnu.*)
   }
}
