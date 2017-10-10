/*
 * loader/aarch64-rpi/bootboot.c
 *
 * Copyright 2017 Public Domain BOOTBOOT bztsrc@github
 *
 * This file is part of the BOOTBOOT Protocol package.
 * @brief Boot loader for the Raspberry Pi 3+ ARMv8
 *
 */
#define DEBUG 1

/* we don't have stdint.h */
typedef signed char         int8_t;
typedef short int           int16_t;
typedef int                 int32_t;
typedef long int            int64_t;
typedef unsigned char       uint8_t;
typedef unsigned short int  uint16_t;
typedef unsigned int        uint32_t;
typedef unsigned long int   uint64_t;

extern uint8_t _end;

/* implemented in boot.S */
extern void delay(uint32_t cnt);

/* get BOOTBOOT structure */
#define _BOOTBOOT_LOADER 1
#include "../bootboot.h"

/*** ELF64 defines and structs ***/
#define ELFMAG      "\177ELF"
#define SELFMAG     4
#define EI_CLASS    4       /* File class byte index */
#define ELFCLASS64  2       /* 64-bit objects */
#define EI_DATA     5       /* Data encoding byte index */
#define ELFDATA2LSB 1       /* 2's complement, little endian */
#define PT_LOAD     1       /* Loadable program segment */
#define EM_AARCH64  183     /* ARM aarch64 architecture */

typedef struct
{
  unsigned char e_ident[16];/* Magic number and other info */
  uint16_t    e_type;         /* Object file type */
  uint16_t    e_machine;      /* Architecture */
  uint32_t    e_version;      /* Object file version */
  uint64_t    e_entry;        /* Entry point virtual address */
  uint64_t    e_phoff;        /* Program header table file offset */
  uint64_t    e_shoff;        /* Section header table file offset */
  uint32_t    e_flags;        /* Processor-specific flags */
  uint16_t    e_ehsize;       /* ELF header size in bytes */
  uint16_t    e_phentsize;    /* Program header table entry size */
  uint16_t    e_phnum;        /* Program header table entry count */
  uint16_t    e_shentsize;    /* Section header table entry size */
  uint16_t    e_shnum;        /* Section header table entry count */
  uint16_t    e_shstrndx;     /* Section header string table index */
} Elf64_Ehdr;

typedef struct
{
  uint32_t    p_type;         /* Segment type */
  uint32_t    p_flags;        /* Segment flags */
  uint64_t    p_offset;       /* Segment file offset */
  uint64_t    p_vaddr;        /* Segment virtual address */
  uint64_t    p_paddr;        /* Segment physical address */
  uint64_t    p_filesz;       /* Segment size in file */
  uint64_t    p_memsz;        /* Segment size in memory */
  uint64_t    p_align;        /* Segment alignment */
} Elf64_Phdr;

/*** PE32+ defines and structs ***/
#define MZ_MAGIC                    0x5a4d      /* "MZ" */
#define PE_MAGIC                    0x00004550  /* "PE\0\0" */
#define IMAGE_FILE_MACHINE_ARM64    0xaa64      /* ARM aarch64 architecture */
#define PE_OPT_MAGIC_PE32PLUS       0x020b      /* PE32+ format */
typedef struct
{
  uint16_t magic;         /* MZ magic */
  uint16_t reserved[29];  /* reserved */
  uint32_t peaddr;        /* address of pe header */
} mz_hdr;

typedef struct {
  uint32_t magic;         /* PE magic */
  uint16_t machine;       /* machine type */
  uint16_t sections;      /* number of sections */
  uint32_t timestamp;     /* time_t */
  uint32_t sym_table;     /* symbol table offset */
  uint32_t symbols;       /* number of symbols */
  uint16_t opt_hdr_size;  /* size of optional header */
  uint16_t flags;         /* flags */
  uint16_t file_type;     /* file type, PE32PLUS magic */
  uint8_t  ld_major;      /* linker major version */
  uint8_t  ld_minor;      /* linker minor version */
  uint32_t text_size;     /* size of text section(s) */
  uint32_t data_size;     /* size of data section(s) */
  uint32_t bss_size;      /* size of bss section(s) */
  uint32_t entry_point;   /* file offset of entry point */
  uint32_t code_base;     /* relative code addr in ram */
} pe_hdr;

#define NULL ((void*)0)
#define PAGESIZE 4096

/*** Raspberry Pi specific defines ***/
#define MMIO_BASE       0x3F000000
#define ARM_TIMER_CTL   (MMIO_BASE+0x0000B408)
#define ARM_TIMER_CNT   (MMIO_BASE+0x0000B420)

#define GPFSEL1         (MMIO_BASE+0x00200004)
#define GPSET0          (MMIO_BASE+0x0020001C)
#define GPCLR0          (MMIO_BASE+0x00200028)
#define GPPUD           (MMIO_BASE+0x00200094)
#define GPPUDCLK0       (MMIO_BASE+0x00200098)

#if DEBUG
#define AUX_ENABLE      (MMIO_BASE+0x00215004)
#define AUX_MU_IO       (MMIO_BASE+0x00215040)
#define AUX_MU_IER      (MMIO_BASE+0x00215044)
#define AUX_MU_IIR      (MMIO_BASE+0x00215048)
#define AUX_MU_LCR      (MMIO_BASE+0x0021504C)
#define AUX_MU_MCR      (MMIO_BASE+0x00215050)
#define AUX_MU_LSR      (MMIO_BASE+0x00215054)
#define AUX_MU_MSR      (MMIO_BASE+0x00215058)
#define AUX_MU_SCRATCH  (MMIO_BASE+0x0021505C)
#define AUX_MU_CNTL     (MMIO_BASE+0x00215060)
#define AUX_MU_STAT     (MMIO_BASE+0x00215064)
#define AUX_MU_BAUD     (MMIO_BASE+0x00215068)

/* UART stuff */
char uart_recv () { while(!(*((uint32_t*)AUX_MU_LSR)&0x01)); return (char)(*((uint32_t*)AUX_MU_IO)&0xFF); }
void uart_send (uint32_t c) { while(!(*((uint32_t*)AUX_MU_LSR)&0x20)); *((uint32_t*)AUX_MU_IO)=c; }
void uart_flush () { while(*((uint32_t*)AUX_MU_LSR)&0x100); }
#define uart_getc uart_recv
void uart_putc(char c) { uart_send((uint32_t)c); if(c=='\n') uart_send((uint32_t)'\r'); }
void uart_puts(const char *s) { while(*s) uart_putc(*s++); }
#define DBG(s,...)
#else
#define DBG(s,...)
#endif

/* string.h */
uint32_t strlen(unsigned char *s) { uint32_t n=0; while(*s++) n++; return n; }
void memcpy(void *dst, void *src, uint32_t n){uint8_t *a=dst,*b=src;while(n--) *a++=*b++; }
int memcmp(void *s1, void *s2, uint32_t n){uint8_t *a=s1,*b=s2;while(n--){if(*a!=*b){return *a-*b;}a++;b++;} return 0; }
/* other string functions */
int atoi(unsigned char *c) { int r=0;while(*c>='0'&&*c<='9') {r*=10;r+=*c++-'0';} return r; }
int oct2bin(unsigned char *s, int n){ int r=0;while(n-->0){r<<=3;r+=*s++-'0';} return r; }
int hex2bin(unsigned char *s, int n){ int r=0;while(n-->0){r<<=4;
    if(*s>='0' && *s<='9')r+=*s-'0';else if(*s>='A'&&*s<='F')r+=*s-'A'+10;s++;} return r; }

#include "tinf.h"
// get filesystem drivers for initrd
#include "../../etc/include/fsZ.h"
#include "fs.h"

/*** other defines and structs ***/

/*** common variables ***/
file_t env;         // environment file descriptor
file_t initrd;      // initrd file descriptor
file_t core;        // kernel file descriptor
BOOTBOOT *bootboot; // the BOOTBOOT structure

// default environment variables. M$ states that 1024x768 must be supported
int reqwidth = 1024, reqheight = 768;
char *kernelname="sys/core";

// alternative environment name
char *cfgname="sys/config";

/**
 * bootboot entry point
 */
void bootboot_main()
{
#if DEBUG
    /* initialize UART */
    uint32_t r;
    *((uint32_t*)AUX_ENABLE) |=1;       // enable mini uart
    *((uint32_t*)AUX_MU_IER) = 0;
    *((uint32_t*)AUX_MU_CNTL) = 0;
    *((uint32_t*)AUX_MU_LCR) = 3;   // 8 bits
    *((uint32_t*)AUX_MU_MCR) = 0;
    *((uint32_t*)AUX_MU_IER) = 0;
    *((uint32_t*)AUX_MU_IIR) = 0xc6;// disable interrupts
    *((uint32_t*)AUX_MU_BAUD) = 270;// 115200 baud
    r=*((uint32_t*)AUX_MU_MCR);
    r&=~((7<<12)|(7<<15));              // gpio14, gpio15
    r|=(2<<12)|(2<<15);                 // alt5
    *((uint32_t*)GPFSEL1) = r;
    *((uint32_t*)GPPUD) = 0;
    delay(150);
    *((uint32_t*)GPPUDCLK0) = (1<<14)|(1<<15);
    delay(150);
    *((uint32_t*)GPPUDCLK0) = 0;    // flush GPIO setup
    *((uint32_t*)AUX_MU_CNTL) = 3;  // enable Tx, Rx
    uart_puts("Booting OS...\n");
#endif
    bootboot = (BOOTBOOT*)&_end;
    memcpy(&bootboot->magic, "BOOT", 4);
    bootboot->protocol = PROTOCOL_STATIC;
    bootboot->loader_type = LOADER_RPI;
    bootboot->size = 128;
    bootboot->pagesize = 4096;
}
