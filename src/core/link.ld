/*
 * src/core/link.ld
 *
 * Copyright 2017 CC-by-nc-sa bztsrc@github
 * https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * You are free to:
 *
 * - Share — copy and redistribute the material in any medium or format
 * - Adapt — remix, transform, and build upon the material
 *     The licensor cannot revoke these freedoms as long as you follow
 *     the license terms.
 *
 * Under the following terms:
 *
 * - Attribution — You must give appropriate credit, provide a link to
 *     the license, and indicate if changes were made. You may do so in
 *     any reasonable manner, but not in any way that suggests the
 *     licensor endorses you or your use.
 * - NonCommercial — You may not use the material for commercial purposes.
 * - ShareAlike — If you remix, transform, or build upon the material,
 *     you must distribute your contributions under the same license as
 *     the original.
 *
 * @brief Linker script for higher half mapped OS/Z core
 */

CORE_ADDRESS = 0xffffffffffe00000;

ENTRY(_start)

PHDRS
{
  text PT_LOAD FILEHDR PHDRS;
}

SECTIONS
{
    .  = CORE_ADDRESS;
    bootboot = .;
    . += 4096;
    environment = .;
    . += 4096;
    .text . + SIZEOF_HEADERS : AT(ADDR(.text) - . + SIZEOF_HEADERS) {
        PROVIDE(_code = .);
        KEEP(*(.text.boot)) *(.text .text.* .gnu.linkonce.t*)   /* code */
        *(.rodata .rodata.* .gnu.linkonce.r*)                   /* data */
        . = ALIGN(4096);
        PROVIDE(_data = .);
        *(.data .data.* .gnu.linkonce.d*)
        srandinitend = .;
    } :text
    /* GNU ld bug workaround. It refuses to place symtab and strtab in text */
    . += 0x4000;
    .bss (NOLOAD) : {
        . = ALIGN(16);
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    } :text

    /* dynamically allocated/mapped data */
    . = ALIGN(4096);
    __dynbss_start = .;
    /* virtual mappings hack */
    tmpctrl = .;
    . += 4096;
    tmpmqctrl = .;
    . += 4096;
    tmppte = .;
    . += 4096;
    tmpmap = .;
    . += 4096;
    tmp2map = .;
    . += 4096;
    tmpalarm = .;
    . += 4096;
    tmpfx = .;
    . += 4096;
    tmp3map = .;
    . += 4096;
    /* CPU Control Block. We need static address for that */
    ccb = .;
    . += 4096;
    /* physical memory manager free memory entries */
    pmm_entries = .;

    /DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.eh*) *(.interp) }
}
