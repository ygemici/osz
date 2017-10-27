/*
 * core/aarch64/libc.c
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
 * @brief low level kernel routines for the core
 */

#include "arch.h"

extern int kx, ky;
extern char kpanicprefix[], kpanicsuffix[], kpanicsuffix2[], kpanicrip[];
extern void kprintf_center(int w, int h);
extern void kprintf_putlogo();
extern void vkprintf(char *fmt, va_list args);
extern uchar *elf_sym(virt_t addr);

int kstrlen(char *s) { uint32_t n=0; while(*s++) n++; return n; }
void kmemcpy(void *dst, void *src, int n){uint8_t *a=dst,*b=src;while(n--) *a++=*b++; }
void kmemset(void *dst, int c, int n){uint8_t *a=dst;while(n--) *a++=c; }
int kmemcmp(void *s1, void *s2, int n){uint8_t *a=s1,*b=s2;while(n--){if(*a!=*b){return *a-*b;}a++;b++;} return 0; }
int kstrcmp(void *s1, void *s2){uint8_t *a=s1,*b=s2;do{if(*a!=*b){return *a-*b;}a++;b++;}while(*a!=0); return 0; }
char *kstrcpy(char *dst, char *src){while(*src) {*dst++=*src++;} *dst=0; return dst; }
/* shuffle bits of random seed */
void kentropy() {
    srand[(uint8_t)(srand[0]>>16)%4]*=16807;
    srand[(uint8_t)srand[0]%4]<<=1; srand[(uint8_t)(srand[0]>>8)%4]>>=1;
    srand[(uint8_t)srand[1]%4]^=srand[((uint8_t)srand[2])%4];
    srand[(uint8_t)srand[3]%4]*=16807;
    srand[0]^=srand[1]; srand[1]^=srand[2]; srand[2]^=srand[3]; srand[3]^=srand[0];
}
/* kernel panic */
extern void kpanic(char *reason, ...) {
    uint32_t c=0;
    va_list args;
    va_start(args, reason);
    kprintf_fade();
    kprintf_fg(0xFFDD33); kprintf_bg(0);
#if DEBUG
    dbg_putchar('\n');
#endif
    kprintf(kpanicprefix);
    vkprintf(reason,args);
#if DEBUG
    kprintf(kpanicrip,((tcb_t*)0)->pc);
    elf_sym(((tcb_t*)0)->pc);
    dbg_putchar('\n');
    dbg_putchar('\n');
    dbg_putchar(27);
    dbg_putchar('[');
    dbg_putchar('7');
    dbg_putchar('m');
#endif
    kprintf_center(54, 5);
    kprintf_fg(0x9c3c1b); kprintf_bg(0x100000);
    kprintf("%s",kpanicsuffix);
    kprintf_fg(0x500000);
    kprintf("%s",kpanicsuffix2);
#if DEBUG
    dbg_putchar(27);
    dbg_putchar('[');
    dbg_putchar('0');
    dbg_putchar('m');
#endif
    kx+=46; ky-=7;
    kprintf_putlogo();
    while(c!=' ' && c!='\n') c=platform_waitkey();
    kprintf_reboot();
}

