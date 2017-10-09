
/* Copyright (C) 2014-2016 Free Software Foundation, Inc.
   Copying and distribution of this script, with or without modification,
   are permitted in any medium without royalty provided the copyright
   notice and this notice are preserved.  */
OUTPUT_FORMAT("elf64-littleaarch64")
OUTPUT_ARCH(aarch64)
ENTRY(_start)
SEARCH_DIR("../bin/initrd/sys/lib");
SEARCH_DIR("../../bin/initrd/sys/lib");
SEARCH_DIR("../../../bin/initrd/sys/lib");
STARTUP("crt0.o");

PHDRS
{
  text PT_LOAD FILEHDR PHDRS ;
  data PT_LOAD ;
  dynamic PT_DYNAMIC ;
}

SECTIONS
{
  /* Read-only sections, merged into text segment: */
  PROVIDE (__executable_start = SEGMENT_START("text-segment", 0));
  . = SEGMENT_START("text-segment", 0);
  .text SIZEOF_HEADERS : AT(SIZEOF_HEADERS)
  {
    _code = .;
    /* text segment must start with crt0 code */
    *(.crt0)
    *(.text.unlikely .text.*_unlikely .text.unlikely.*)
    *(.text.exit .text.exit.*)
    *(.text.startup .text.startup.*)
    *(.text.hot .text.hot.*)
    *(.text .stub .text.* .gnu.linkonce.t.*)
  } :text =0x90
  /* important that dynstr follows dynsym */
  .dynsym         : { *(.dynsym) }
  .dynstr         : { *(.dynstr) }
  .rela.dyn       :
    {
      *(.rela.init)
      *(.rela.text .rela.text.* .rela.gnu.linkonce.t.*)
      *(.rela.fini)
      *(.rela.rodata .rela.rodata.* .rela.gnu.linkonce.r.*)
      *(.rela.data .rela.data.* .rela.gnu.linkonce.d.*)
      *(.rela.tdata .rela.tdata.* .rela.gnu.linkonce.td.*)
      *(.rela.tbss .rela.tbss.* .rela.gnu.linkonce.tb.*)
      *(.rela.ctors)
      *(.rela.dtors)
      *(.rela.got)
      *(.rela.bss .rela.bss.* .rela.gnu.linkonce.b.*)
      *(.rela.ldata .rela.ldata.* .rela.gnu.linkonce.l.*)
      *(.rela.lbss .rela.lbss.* .rela.gnu.linkonce.lb.*)
      *(.rela.lrodata .rela.lrodata.* .rela.gnu.linkonce.lr.*)
      *(.rela.ifunc)
    }
  .rela.plt       :
    {
      *(.rela.plt)
      PROVIDE_HIDDEN (__rela_iplt_start = .);
      *(.rela.iplt)
      PROVIDE_HIDDEN (__rela_iplt_end = .);
    }
  .plt            : { *(.plt) *(.iplt) }
  .plt.got        : { *(.plt.got) }
  .plt.bnd        : { *(.plt.bnd) }
  PROVIDE (_etext = .);
  .buildid        : { *(.buildid) *(.note.gnu.build-id) }
  .rodata         : { *(.rodata .rodata.* .gnu.linkonce.r.*) }
  .rodata1        : { *(.rodata1) }
  . = ALIGN(4096);
  . = DATA_SEGMENT_ALIGN (4096, 4096);
  PROVIDE (_data = .);
  .data           :
  {
    *(.data .data.* .gnu.linkonce.d.*)
    SORT(CONSTRUCTORS)
   *(.bss .bss.* .gnu.linkonce.b.*)
   *(COMMON)
   . = ALIGN(. != 0 ? 64 / 8 : 1);
  } :data
  .data1          : { *(.data1) }
  .data.rel.ro : { *(.data.rel.ro.local* .gnu.linkonce.d.rel.ro.local.*) *(.data.rel.ro .data.rel.ro.* .gnu.linkonce.d.rel.ro.*) }
  .dynamic        : { *(.dynamic) } :data :dynamic
  . = ALIGN(8);
  .got            : { *(.got) *(.igot) } :data
  .got.plt        : { *(.got.plt)  *(.igot.plt) }
  PROVIDE (_edata = .);
  __bss_start = 0x100000000;
  .bss            :
  {
   *(.dynbss)
  }
  _end = .;
  . = DATA_SEGMENT_END (.);
  /* Stabs debugging sections.  */
  .stab          0 : { *(.stab) }
  .stabstr       0 : { *(.stabstr) }
  .stab.excl     0 : { *(.stab.excl) }
  .stab.exclstr  0 : { *(.stab.exclstr) }
  .stab.index    0 : { *(.stab.index) }
  .stab.indexstr 0 : { *(.stab.indexstr) }
  /DISCARD/ : { *(.note.*) *(.gnu_debuglink) *(.gnu.lto_*) *(.interp) *(.eh*) *(.comment*) }
}
