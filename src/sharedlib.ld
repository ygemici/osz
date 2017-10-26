/*
 * src/sharedlib.ld
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
 * @brief Linker script for shared objects
 */

ENTRY(_init)

PHDRS
{
  text PT_LOAD FILEHDR PHDRS;
  data PT_LOAD;
  dynamic PT_DYNAMIC;
}

SECTIONS
{
  . = SEGMENT_START("text-segment", 0) + SIZEOF_HEADERS;
  .text : { *(.text .text.* .stub .gnu.linkonce.t.*) *(.plt) *(.iplt) *(.plt.got) *(.plt.bnd) } :text =0x90
  .rodata : { *(.rodata .rodata.* .gnu.linkonce.r.*) }
  .dynsym : { *(.dynsym) }
  .dynstr : { *(.dynstr) }
  .rela.dyn : { *(.rela*) }
  .rela.plt : { *(.rela.plt) *(rela.iplt) }

  . = ALIGN(4096);
  .got : { *(.got) *(.igot) } :data
  .got.plt : { *(.got.plt)  *(.igot.plt) }
  .dynamic : { *(.dynamic) } :data :dynamic
  .data : { *(.data .data.* .gnu.linkonce.d.*) } :data
  /* GNU ld bug workaround: keep symtab and strtab */
  . += 4096;
  .bss (NOLOAD) : { . = ALIGN(16); *(.bss .bss.* .sbss* .gnu.linkonce.b.*) *(COMMON) } :data

  /* Stabs debugging sections.  */
  .stab          0 : { *(.stab) }
  .stabstr       0 : { *(.stabstr) }
  .stab.excl     0 : { *(.stab.excl) }
  .stab.exclstr  0 : { *(.stab.exclstr) }
  .stab.index    0 : { *(.stab.index) }
  .stab.indexstr 0 : { *(.stab.indexstr) }

  /DISCARD/ : { *(.note*) *(.gnu*) *(.interp) *(.eh*) *(.comment*) }
}
