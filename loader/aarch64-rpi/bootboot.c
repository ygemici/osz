/*
 * loader/aarch64-rpi/bootboot.c
 *
 * Copyright 2017 Public Domain BOOTBOOT bztsrc@github
 *
 * This file is part of the BOOTBOOT Protocol package.
 * @brief Boot loader for the Raspberry Pi 3+ ARMv8
 *
 * memory occupied: 0-0x10000
 * 
 * Memory map
 *           0x0 - 0x2000         stack
 *        0x2000 - __bss_start    loader code and data
 *         bss+x - bss+x+0x1000   bootboot structure
 *  bss+x+0x1000 - bss+x+0x2000   environment
 *  bss+x+0x2000 - 0x10000        memory mapping structures
 *       0x10000 - x              initrd or compressed initrd
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

/* aligned buffers */
extern uint8_t __mbr;
// address of mailbox in memory
extern uint32_t __mailbox;
// address of bootboot struct in memory
extern uint8_t __bootboot;
// address of environment in memory
extern uint8_t __environment;

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
#define STMR_L          ((volatile uint32_t*)(MMIO_BASE+0x00003004))
#define STMR_H          ((volatile uint32_t*)(MMIO_BASE+0x00003008))

#define ARM_TIMER_CTL   ((volatile uint32_t*)(MMIO_BASE+0x0000B408))
#define ARM_TIMER_CNT   ((volatile uint32_t*)(MMIO_BASE+0x0000B420))

#define PM_RTSC         ((volatile uint32_t*)(MMIO_BASE+0x0010001c))
#define PM_WATCHDOG     ((volatile uint32_t*)(MMIO_BASE+0x00100024))
#define PM_WDOG_MAGIC   0x5a000000
#define PM_RTSC_FULLRST 0x00000020

#define GPFSEL0         ((volatile uint32_t*)(MMIO_BASE+0x00200000))
#define GPFSEL1         ((volatile uint32_t*)(MMIO_BASE+0x00200004))
#define GPFSEL2         ((volatile uint32_t*)(MMIO_BASE+0x00200008))
#define GPFSEL3         ((volatile uint32_t*)(MMIO_BASE+0x0020000C))
#define GPFSEL4         ((volatile uint32_t*)(MMIO_BASE+0x00200010))
#define GPFSEL5         ((volatile uint32_t*)(MMIO_BASE+0x00200014))
#define GPSET0          ((volatile uint32_t*)(MMIO_BASE+0x0020001C))
#define GPSET1          ((volatile uint32_t*)(MMIO_BASE+0x00200020))
#define GPCLR0          ((volatile uint32_t*)(MMIO_BASE+0x00200028))
#define GPLEV0          ((volatile uint32_t*)(MMIO_BASE+0x00200034))
#define GPLEV1          ((volatile uint32_t*)(MMIO_BASE+0x00200038))
#define GPEDS0          ((volatile uint32_t*)(MMIO_BASE+0x00200040))
#define GPEDS1          ((volatile uint32_t*)(MMIO_BASE+0x00200044))
#define GPHEN0          ((volatile uint32_t*)(MMIO_BASE+0x00200064))
#define GPHEN1          ((volatile uint32_t*)(MMIO_BASE+0x00200068))
#define GPPUD           ((volatile uint32_t*)(MMIO_BASE+0x00200094))
#define GPPUDCLK0       ((volatile uint32_t*)(MMIO_BASE+0x00200098))
#define GPPUDCLK1       ((volatile uint32_t*)(MMIO_BASE+0x0020009C))

#define AUX_ENABLE      ((volatile uint32_t*)(MMIO_BASE+0x00215004))
#define AUX_MU_IO       ((volatile uint32_t*)(MMIO_BASE+0x00215040))
#define AUX_MU_IER      ((volatile uint32_t*)(MMIO_BASE+0x00215044))
#define AUX_MU_IIR      ((volatile uint32_t*)(MMIO_BASE+0x00215048))
#define AUX_MU_LCR      ((volatile uint32_t*)(MMIO_BASE+0x0021504C))
#define AUX_MU_MCR      ((volatile uint32_t*)(MMIO_BASE+0x00215050))
#define AUX_MU_LSR      ((volatile uint32_t*)(MMIO_BASE+0x00215054))
#define AUX_MU_MSR      ((volatile uint32_t*)(MMIO_BASE+0x00215058))
#define AUX_MU_SCRATCH  ((volatile uint32_t*)(MMIO_BASE+0x0021505C))
#define AUX_MU_CNTL     ((volatile uint32_t*)(MMIO_BASE+0x00215060))
#define AUX_MU_STAT     ((volatile uint32_t*)(MMIO_BASE+0x00215064))
#define AUX_MU_BAUD     ((volatile uint32_t*)(MMIO_BASE+0x00215068))

/* timing stuff */
uint64_t getmicro(void){uint32_t h=*STMR_H,l=*STMR_L;if(h!=*STMR_H){h=*STMR_H;l=*STMR_L;} return(((uint64_t)h)<<32)|l;}
/* delay cnt clockcycles */
void delay(uint32_t cnt) { while(cnt--) { asm volatile("nop"); } }
/* delay cnt microsec */
void delaym(uint32_t cnt) { uint64_t t=getmicro();cnt+=t;while(getmicro()<cnt); }

/* UART stuff */
void uart_send(uint32_t c) { do{asm volatile("nop");}while(!(*AUX_MU_LSR&0x20)); *AUX_MU_IO=c; }
void uart_hex(uint64_t d,int c) { uint32_t n;c<<=3;c-=4;for(;c>=0;c-=4){n=(d>>c)&0xF;n+=n>9?0x37:0x30;uart_send(n);} }
void uart_putc(char c) { if(c=='\n') uart_send((uint32_t)'\r'); uart_send((uint32_t)c); }
void uart_puts(char *s) { while(*s) uart_putc(*s++); }
char uart_getc() {char r;do{asm volatile("nop");}while(!(*AUX_MU_LSR&0x01));r=(char)(*AUX_MU_IO);return r=='\r'?'\n':r;}
void uart_dump(void *ptr,uint32_t l) {
    uint64_t a,b;
    unsigned char c;
    for(a=(uint64_t)ptr;a<(uint64_t)ptr+l*16;a+=16) {
        uart_hex(a,4); uart_puts(": ");
        for(b=0;b<16;b++) {
            uart_hex(*((unsigned char*)(a+b)),1);
            uart_putc(' ');
            if(b%4==3)
                uart_putc(' ');
        }
        for(b=0;b<16;b++) {
            c=*((unsigned char*)(a+b));
            uart_putc(c<32||c>=127?'.':c);
        }
        uart_putc('\n');
    }
}

#define VIDEOCORE_MBOX  (MMIO_BASE+0x0000B880)
#define MBOX_READ       ((volatile uint32_t*)(VIDEOCORE_MBOX+0x0))
#define MBOX_POLL       ((volatile uint32_t*)(VIDEOCORE_MBOX+0x10))
#define MBOX_SENDER     ((volatile uint32_t*)(VIDEOCORE_MBOX+0x14))
#define MBOX_STATUS     ((volatile uint32_t*)(VIDEOCORE_MBOX+0x18))
#define MBOX_CONFIG     ((volatile uint32_t*)(VIDEOCORE_MBOX+0x1C))
#define MBOX_WRITE      ((volatile uint32_t*)(VIDEOCORE_MBOX+0x20))
#define MBOX_REQUEST    0
#define MBOX_RESPONSE   0x80000000
#define MBOX_FULL       0x80000000
#define MBOX_EMPTY      0x40000000
#define MBOX_CH_POWER   0
#define MBOX_CH_FB      1
#define MBOX_CH_VUART   2
#define MBOX_CH_VCHIQ   3
#define MBOX_CH_LEDS    4
#define MBOX_CH_BTNS    5
#define MBOX_CH_TOUCH   6
#define MBOX_CH_COUNT   7
#define MBOX_CH_PROP    8

/* mailbox functions */
void mbox_write(uint8_t ch, volatile uint32_t *mbox)
{
    do{asm volatile("nop");}while(*MBOX_STATUS & MBOX_FULL);
    *MBOX_WRITE = (((uint32_t)((uint64_t)mbox)&~0xF) | (ch&0xF));
}
uint32_t mbox_read(uint8_t ch)
{
    uint32_t r;
    while(1) {
        do{asm volatile("nop");}while(*MBOX_STATUS & MBOX_EMPTY);
        r=*MBOX_READ;
        if((uint8_t)(r&0xF)==ch)
            return (r&~0xF);
    }
}
uint8_t mbox_call(uint8_t ch, volatile uint32_t *mbox)
{
    mbox_write(ch,mbox);
    return mbox_read(ch)==(uint32_t)((uint64_t)mbox) && mbox[1]==MBOX_RESPONSE;
}

/* sdcard */
#define EMMC_ARG2           ((volatile uint32_t*)(MMIO_BASE+0x00300000))
#define EMMC_BLKSIZECNT     ((volatile uint32_t*)(MMIO_BASE+0x00300004))
#define EMMC_ARG1           ((volatile uint32_t*)(MMIO_BASE+0x00300008))
#define EMMC_CMDTM          ((volatile uint32_t*)(MMIO_BASE+0x0030000C))
#define EMMC_RESP0          ((volatile uint32_t*)(MMIO_BASE+0x00300010))
#define EMMC_RESP1          ((volatile uint32_t*)(MMIO_BASE+0x00300014))
#define EMMC_RESP2          ((volatile uint32_t*)(MMIO_BASE+0x00300018))
#define EMMC_RESP3          ((volatile uint32_t*)(MMIO_BASE+0x0030001C))
#define EMMC_DATA           ((volatile uint32_t*)(MMIO_BASE+0x00300020))
#define EMMC_STATUS         ((volatile uint32_t*)(MMIO_BASE+0x00300024))
#define EMMC_CONTROL0       ((volatile uint32_t*)(MMIO_BASE+0x00300028))
#define EMMC_CONTROL1       ((volatile uint32_t*)(MMIO_BASE+0x0030002C))
#define EMMC_INTERRUPT      ((volatile uint32_t*)(MMIO_BASE+0x00300030))
#define EMMC_INT_MASK       ((volatile uint32_t*)(MMIO_BASE+0x00300034))
#define EMMC_INT_EN         ((volatile uint32_t*)(MMIO_BASE+0x00300038))
#define EMMC_CONTROL2       ((volatile uint32_t*)(MMIO_BASE+0x0030003C))
#define EMMC_SLOTISR_VER    ((volatile uint32_t*)(MMIO_BASE+0x003000FC))

// command flags
#define CMD_IS_DATA         0x00200000
#define CMD_RSPNS_48        0x00020000
#define TM_DAT_DIR_CH       0x00000010

// COMMANDs
#define CMD_READ_SINGLE     0x11220010

// STATUS register settings
#define SR_DAT_INHIBIT      0x00000002
#define SR_CMD_INHIBIT      0x00000001

// INTERRUPT register settings
#define INT_DATA_TIMEOUT    0x00100000
#define INT_CMD_TIMEOUT     0x00010000
#define INT_READ_RDY        0x00000020
#define INT_CMD_DONE        0x00000001

#define INT_ERROR_MASK      0x017E8000

// CONTROL register settings
#define C0_SPI_MODE_EN      0x00100000
#define C0_HCTL_HS_EN       0x00000004
#define C0_HCTL_DWITDH      0x00000002

#define C1_SRST_DATA        0x04000000
#define C1_SRST_CMD         0x02000000
#define C1_SRST_HC          0x01000000
#define C1_TOUNIT_DIS       0x000f0000
#define C1_TOUNIT_MAX       0x000e0000
#define C1_CLK_GENSEL       0x00000020
#define C1_CLK_EN           0x00000004
#define C1_CLK_STABLE       0x00000002
#define C1_CLK_INTLEN       0x00000001

#define SD_OK               0
#define SD_TIMEOUT          -1
#define SD_ERROR            -2

uint8_t sd_init=0;

/**
 * Wait for data
 */
int sdWaitData()
{
uart_puts("sdWaitData\n");
    int cnt = 500000;
    while((*EMMC_STATUS & SR_DAT_INHIBIT) && !(*EMMC_INTERRUPT & INT_ERROR_MASK) && cnt--) delaym(1);
uart_puts(" cnt ");uart_hex(cnt,4);uart_putc(' ');
uart_puts(" EMMC_STATUS ");uart_hex(*EMMC_STATUS,4);uart_putc(' ');
uart_puts(" EMMC_INT ");uart_hex(*EMMC_INTERRUPT,4);uart_putc('\n');
    if(cnt <= 0 || (*EMMC_INTERRUPT & INT_ERROR_MASK)) return SD_ERROR;
    return SD_OK;
}

/**
 * Wait for interrupt
 */
int sdWaitInt( uint32_t mask )
{
uart_puts("sdWaitInt ");
uart_hex(mask,4);
uart_putc('\n');
    int cnt = 1000000;
    uint32_t r, m=mask | INT_ERROR_MASK;
    while(!(*EMMC_INTERRUPT & m) && cnt--) delaym(1);
    r=*EMMC_INTERRUPT;
uart_puts(" cnt ");uart_hex(cnt,4);uart_putc(' ');
uart_puts(" EMMC_INT ");uart_hex(r,4);uart_putc('\n');
    if(cnt<=0 || (r & INT_CMD_TIMEOUT) || (r & INT_DATA_TIMEOUT) ) { *EMMC_INTERRUPT=r; return SD_TIMEOUT; } else
    if(r & INT_ERROR_MASK) { *EMMC_INTERRUPT=r; return SD_ERROR; }
    *EMMC_INTERRUPT=mask;
    return 0;
}

/**
 * Send a command
 */
int sdCmd(uint32_t code, uint32_t arg)
{
uart_puts("sdCmd ");
uart_hex(code,4);
uart_putc(' ');
uart_hex(arg,4);
uart_putc('\n');
    // Check for status indicating a command in progress.
    int r,cnt = 1000000;
    while((*EMMC_STATUS & SR_CMD_INHIBIT) && !(*EMMC_INTERRUPT & INT_ERROR_MASK) && cnt--) delaym(1);
uart_puts(" cnt ");uart_hex(cnt,4);uart_putc(' ');
uart_puts(" EMMC_STATUS ");uart_hex(*EMMC_STATUS,4);uart_putc(' ');
uart_puts(" EMMC_INT ");uart_hex(*EMMC_INTERRUPT,4);uart_putc('\n');
    if(cnt<=0 || (*EMMC_INTERRUPT & INT_ERROR_MASK)) return SD_ERROR;
    
    *EMMC_INTERRUPT=*EMMC_INTERRUPT;
    *EMMC_ARG1=arg;
    *EMMC_CMDTM=code;
    if((r=sdWaitInt(INT_CMD_DONE))) return r;
    return *EMMC_RESP0;
}

int sdInit()
{
    uint32_t r;
    int cnt;
    sd_init=0;
uart_puts("sdInit\n");
    // GPIO_CD
    r=*GPFSEL4; r&=~(7<<(7*3)); *GPFSEL4=r;
    *GPPUD=2; delay(150); *GPPUDCLK1=(1<<15); delay(150); *GPPUD=0; *GPPUDCLK1=0;

    r=*GPHEN1; r|=1<<15; *GPHEN1=r;

    // GPIO_CLK
    r=*GPFSEL4; r&=~(7<<(8*3)); r|=(7<<(8*3)); *GPFSEL4=r;
    *GPPUD=2; delay(150); *GPPUDCLK1=(1<<16); delay(150); *GPPUD=0; *GPPUDCLK1=0;

    // GPIO_CMD
    r=*GPFSEL4; r&=~(7<<(9*3)); r|=(7<<(9*3)); *GPFSEL4=r;
    *GPPUD=2; delay(150); *GPPUDCLK1=(1<<17); delay(150); *GPPUD=0; *GPPUDCLK1=0;

    // GPIO_DAT0
    r=*GPFSEL5; r&=~(7<<(0*3)); r|=(7<<(0*3)); *GPFSEL5=r;
    *GPPUD=2; delay(150); *GPPUDCLK1=(1<<18); delay(150); *GPPUD=0; *GPPUDCLK1=0;
    
    // GPIO_DAT1
    r=*GPFSEL5; r&=~(7<<(1*3)); r|=(7<<(1*3)); *GPFSEL5=r;
    *GPPUD=2; delay(150); *GPPUDCLK1=(1<<19); delay(150); *GPPUD=0; *GPPUDCLK1=0;

    // GPIO_DAT2
    r=*GPFSEL5; r&=~(7<<(2*3)); r|=(7<<(2*3)); *GPFSEL5=r;
    *GPPUD=2; delay(150); *GPPUDCLK1=(1<<20); delay(150); *GPPUD=0; *GPPUDCLK1=0;

    // GPIO_DAT3
    r=*GPFSEL5; r&=~(7<<(3*3)); r|=(7<<(3*3)); *GPFSEL5=r;
    *GPPUD=2; delay(150); *GPPUDCLK1=(1<<21); delay(150); *GPPUD=0; *GPPUDCLK1=0;

uart_puts("GPFSEL4,5 ");
uart_hex(*GPFSEL4,4);
uart_putc(' ');
uart_hex(*GPFSEL5,4);
uart_putc('\n');

    // clear ejected flag
    *GPEDS1 |= ~(1 << (47-32));

    *EMMC_INTERRUPT=*EMMC_INTERRUPT;
    // reset
    *EMMC_CONTROL0 = 0; // C0_SPI_MODE_EN;
    *EMMC_CONTROL1 |= C1_SRST_HC;
    delaym(10);
    cnt = 10000;
    while((*EMMC_CONTROL1 & C1_SRST_HC) && cnt--) delaym(10);
    if(cnt<=0) return 0;

    *EMMC_CONTROL1 |= C1_CLK_INTLEN | C1_TOUNIT_MAX;
    delaym(10);

    *EMMC_INT_EN   = 0xffffffff;
    *EMMC_INT_MASK = 0xffffffff;

    sd_init=1;
    return 1;
}

/**
 * read a block from sd card and return the number of bytes read
 * returns 0 on error.
 */
int readblock(uint64_t lba, uint8_t *buffer)
{
    int r;
    uint32_t *buf=(uint32_t*)buffer;
uart_puts("readblock ");
uart_hex(lba,4);
uart_putc(' ');
uart_hex((uint32_t)((uint64_t)buffer),4);
uart_putc('\n');

    if(!sd_init || !sdInit()) return 0;
uart_puts("GPLEV1 ");
uart_hex(*GPLEV1,4);
uart_puts(" GPEDS1 ");
uart_hex(*GPEDS1,4);
uart_putc('\n');
    // sd card absent or ejected?
//    if(!(*GPLEV1 & (1 << (47-32))) || (*GPEDS1 & (1 << (47-32)))) { sd_init=0; return 0; }
//uart_puts(" no eject\n");
    if(!sdWaitData()) return 0;
uart_puts(" after waitdata\n");
    *EMMC_BLKSIZECNT = (1 << 16) | 512;
    if((r=sdCmd(CMD_READ_SINGLE,lba))) return 0;
uart_puts(" after sdCmd\n");
    if((r=sdWaitInt(INT_READ_RDY))) return 0;
uart_puts(" after waitInt\n");
    for(r=0;r<128;r++) buf[r] = *EMMC_DATA;
uart_puts(" read ok\n");
    return 512;
}

/* string.h */
uint32_t strlen(unsigned char *s) { uint32_t n=0; while(*s++) n++; return n; }
void memcpy(void *dst, void *src, uint32_t n){uint8_t *a=dst,*b=src;while(n--) *a++=*b++; }
int memcmp(void *s1, void *s2, uint32_t n){uint8_t *a=s1,*b=s2;while(n--){if(*a!=*b){return *a-*b;}a++;b++;} return 0; }
/* other string functions */
int atoi(unsigned char *c) { int r=0;while(*c>='0'&&*c<='9') {r*=10;r+=*c++-'0';} return r; }
int oct2bin(unsigned char *s, int n){ int r=0;while(n-->0){r<<=3;r+=*s++-'0';} return r; }
int hex2bin(unsigned char *s, int n){ int r=0;while(n-->0){r<<=4;
    if(*s>='0' && *s<='9')r+=*s-'0';else if(*s>='A'&&*s<='F')r+=*s-'A'+10;s++;} return r; }

#if DEBUG
void puts(char *s);
#define DBG(s) puts(s)
#else
#define DBG(s)
#endif

#include "tinf.h"
// get filesystem drivers for initrd
#include "../../etc/include/fsZ.h"
#include "fs.h"

/*** other defines and structs ***/
typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t headersize;/* offset of bitmaps in file */
    uint16_t flags;     /* original PSF2 has 32 bit flags */
    uint8_t hotspot_x;  /* addition to OS/Z */
    uint8_t hotspot_y;
    uint32_t numglyph;
    uint32_t bytesperglyph;
    uint32_t height;
    uint32_t width;
    uint8_t glyphs;
} __attribute__((packed)) font_t;

extern volatile unsigned char _binary_font_psf_start;
/* current cursor position */
int kx, ky;
/* maximum coordinates */
int maxx, maxy;

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
 * Get a linear frame buffer
 */
int GetLFB(uint32_t width, uint32_t height)
{
    font_t *font = (font_t*)&_binary_font_psf_start;
    volatile uint32_t *mbox = &__mailbox;

    //query natural width, height if not given
    if(width==0 && height==0) {
        mbox[0] = 8*4;
        mbox[1] = MBOX_REQUEST;
        mbox[2] = 0x40003;  //get phy wh
        mbox[3] = 8;
        mbox[4] = 8;
        mbox[5] = 0;
        mbox[6] = 0;
        mbox[7] = 0;
        if(mbox_call(MBOX_CH_PROP,mbox) && mbox[5]!=0) {
            width=mbox[5];
            height=mbox[6];
        }
    }
    //if we already have a framebuffer, release it
    if(bootboot->fb_ptr!=NULL) {
        mbox[0] = 8*4;
        mbox[1] = MBOX_REQUEST;
        mbox[2] = 0x48001;  //release buffer
        mbox[3] = 8;
        mbox[4] = 8;
        mbox[5] = (uint32_t)(((uint64_t)bootboot->fb_ptr));
        mbox[6] = (uint32_t)(((uint64_t)bootboot->fb_ptr)>>32);
        mbox[7] = 0;
        mbox_call(MBOX_CH_PROP,mbox);
    }
    //check minimum resolution
    if(width<800 || height<600) {
        width=800; height=600;
    }
    mbox[0] = 31*4;
    mbox[1] = MBOX_REQUEST;

    mbox[2] = 0x48003;  //set phy wh
    mbox[3] = 8;
    mbox[4] = 8;
    mbox[5] = width;
    mbox[6] = height;

    mbox[7] = 0x48004;  //set virt wh
    mbox[8] = 8;
    mbox[9] = 8;
    mbox[10] = width;
    mbox[11] = height;
    
    mbox[12] = 0x48009; //set virt offset
    mbox[13] = 8;
    mbox[14] = 8;
    mbox[15] = 0;
    mbox[16] = 0;
    
    mbox[17] = 0x48005; //set depth
    mbox[18] = 4;
    mbox[19] = 4;
    mbox[20] = 32;      //only RGBA supported

    mbox[21] = 0x40001; //get framebuffer
    mbox[22] = 8;       //response size
    mbox[23] = 8;       //request size
    mbox[24] = PAGESIZE;//buffer alignment
    mbox[25] = 0;
    
    mbox[26] = 0x40008; //get pitch
    mbox[27] = 4;
    mbox[28] = 4;
    mbox[29] = 0;

    mbox[30] = 0;       //Arnold Schwarzenegger

    if(mbox_call(MBOX_CH_PROP,mbox) && mbox[20]==32 && mbox[23]==(MBOX_RESPONSE|8) && mbox[25]!=0) {
        mbox[24]&=0x3FFFFFFF;
        bootboot->fb_width=mbox[5];
        bootboot->fb_height=mbox[6];
        bootboot->fb_scanline=mbox[29];
        bootboot->fb_ptr=(void*)((uint64_t)mbox[24]);
        bootboot->fb_size=mbox[25];
        bootboot->fb_type=FB_ARGB;
        kx=ky=0;
        maxx=bootboot->fb_width/(font->width+1);
        maxy=bootboot->fb_height/font->height;
        return 1;
    }
    return 0;
}

/**
 * display one literal unicode character
 */
void putc(char c)
{
    font_t *font = (font_t*)&_binary_font_psf_start;
    unsigned char *glyph = (unsigned char*)&_binary_font_psf_start +
     font->headersize + (c>0&&c<font->numglyph?c:0)*font->bytesperglyph;
    int offs = (ky * font->height * bootboot->fb_scanline) + (kx * (font->width+1) * 4);
    int x,y, line,mask;
    int bytesperline=(font->width+7)/8;
    for(y=0;y<font->height;y++){
        line=offs;
        mask=1<<(font->width-1);
        for(x=0;x<font->width;x++){
            *((uint32_t*)((uint64_t)bootboot->fb_ptr + line))=((int)*glyph) & (mask)?0xFFFFFF:0;
            mask>>=1;
            line+=4;
        }
        *((uint32_t*)((uint64_t)bootboot->fb_ptr + line))=0;
        glyph+=bytesperline;
        offs+=bootboot->fb_scanline;
    }
    // send it to serial too
    uart_putc(c);
}

/**
 * display a string
 */
void puts(char *s)
{
    while(*s) {
        if(*s=='\r') {
            uart_putc(*s);
            kx=0;
        } else
        if(*s=='\n') {
            uart_putc(*s);
            kx=0;ky++;
        } else {
            putc(*s);
            kx++;
            if(kx>=maxx) {
                kx=0; ky++;
            }
        }
        s++;
    }
}

/**
 * bootboot entry point
 */
int bootboot_main(void)
{
    uint32_t r;

    /* initialize UART */
    *AUX_ENABLE |=1;       // enable mini uart
    *AUX_MU_IER = 0;
    *AUX_MU_CNTL = 0;
    *AUX_MU_LCR = 3;       // 8 bits
    *AUX_MU_MCR = 0;
    *AUX_MU_IER = 0;
    *AUX_MU_IIR = 0xc6;    // disable interrupts
    *AUX_MU_BAUD = 270;    // 115200 baud
    r=*GPFSEL1;
    r&=~((7<<12)|(7<<15)); // gpio14, gpio15
    r|=(2<<12)|(2<<15);    // alt5
    *GPFSEL1 = r;
    *GPPUD = 0;
    delay(150);
    *GPPUDCLK0 = (1<<14)|(1<<15);
    delay(150);
    *GPPUDCLK0 = 0;        // flush GPIO setup
    *AUX_MU_CNTL = 3;      // enable Tx, Rx

    // create bootboot structure
    bootboot = (BOOTBOOT*)&__bootboot;
    bootboot->magic[0]='B';bootboot->magic[1]=bootboot->magic[2]='O';bootboot->magic[3]='T';
    bootboot->protocol = PROTOCOL_STATIC;
    bootboot->loader_type = LOADER_RPI;
    bootboot->size = 128;
    bootboot->pagesize = PAGESIZE;
    // set up a framebuffer so that we can write on screen
    if(!GetLFB(0, 0)) goto viderr;
    puts("Booting OS...\n");

    sdInit();
    r=readblock(0,&__mbr);
    uart_puts("MBR read ret ");
    uart_hex(r,4);
    uart_putc('\n');
    if(r)
        uart_dump((void*)&__mbr,32);

    // load BOOTBOOT/CONFIG

    // load initrd (or check if initramfs command has already loaded it)
    DBG(" * Initrd loaded\n");

    // uncompress if it's compressed
    DBG(" * Gzip compressed initrd\n");
    
    // if no config, locate it in uncompressed initrd

    // parse config
    DBG(" * Environment\n");

    // get linear framebuffer if it's different
    DBG(" * Screen VideoCore\n");
    if(reqwidth!=bootboot->fb_width || reqheight!=bootboot->fb_height) {
#if DEBUG
uart_puts("Fb not match, got");
uart_hex(bootboot->fb_width,2);
uart_putc('x');
uart_hex(bootboot->fb_height,2);
uart_puts(", need ");
uart_hex(reqwidth,2);
uart_putc('x');
uart_hex(reqheight,2);
uart_putc('\n');
#endif
        if(!GetLFB(reqwidth, reqheight)) {
viderr:
            puts("BOOTBOOT-PANIC: VideoCore error, no framebuffer\n");
            goto error;
        }
    }


    // locate sys/core
    DBG(" * Parsing ELF64\n");
    DBG(" * Parsing PE32+\n");
    
    // create memory mapping

    // generate memory map to bootboot struct

    // jump to core's _start

    // echo back everything until Enter pressed
error:
    while(r!='\n' && r!=' ') r=uart_getc();
    uart_puts("\n\n"); delay(1500);

    // reset
    *PM_WATCHDOG = PM_WDOG_MAGIC | 1;
    *PM_RTSC = PM_WDOG_MAGIC | PM_RTSC_FULLRST;
    while(1);
}
