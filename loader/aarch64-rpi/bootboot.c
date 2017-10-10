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

/* implemented in boot.S */
extern void delay(uint32_t cnt);

/* get BOOTBOOT structure */
#define _BOOTBOOT_LOADER 1
#include "../bootboot.h"
extern uint8_t _end;
BOOTBOOT *bootboot = (BOOTBOOT*)&_end;

#define NULL ((void*)0)

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

#include "fs.h"

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
    memcpy(&bootboot->magic, "BOOT", 4);
    bootboot->protocol = PROTOCOL_STATIC;
    bootboot->loader_type = LOADER_RPI;
    bootboot->size = 128;
    bootboot->pagesize = 4096;
}
