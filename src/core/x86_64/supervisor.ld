/* kernel space addresses */
CORE_ADDRESS  = 0xffffffffffe02000;

ENTRY(_start)
OUTPUT_FORMAT(elf64-x86-64)
PHDRS
{
  text PT_LOAD FILEHDR PHDRS FLAGS(5);
}
SECTIONS
{
    /* make sure it starts with bootboot */
    . = CORE_ADDRESS - 8192;
        bootboot = .;
        . += 4096;
        environment = .;
        . += 4096;

    /* put headers in load segment so that we won't have to copy
        text segment on loading as it will be aligned */
    .text CORE_ADDRESS + SIZEOF_HEADERS : AT(ADDR(.text) - CORE_ADDRESS + SIZEOF_HEADERS)
    {
        /* code */
        PROVIDE(_code = .);
        *(.text)
        . = ALIGN(4096);
        /* initialized and preallocated data */
        PROVIDE(_data = .);
        *(.rodata)
        *(.data)
        *(.got*)
        . = ALIGN(4096);
        /* user mode code */
        PROVIDE(_usercode = .);
        *(.text.user)
        . = ALIGN(4096);
    } :text
    .dynamic : { *(.dynamic) } :text :dynamic
    /* GNU ld buf workaround. It refuses to place symtab and strtab in text */
    . += 65536;

    /* dynamically allocated/mapped data */
    __bss_start = .;
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
