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
extern char kpanicprefix[], kpanicsuffix[], kpanicsuffix2[];
extern void kprintf_center(int w, int h);
extern void kprintf_putlogo();

int kstrlen(char *s) { uint32_t n=0; while(*s++) n++; return n; }
void kmemcpy(void *dst, void *src, int n){uint8_t *a=dst,*b=src;while(n--) *a++=*b++; }
void kmemset(void *dst, int c, int n){uint8_t *a=dst;while(n--) *a++=c; }
int kmemcmp(void *s1, void *s2, int n){uint8_t *a=s1,*b=s2;while(n--){if(*a!=*b){return *a-*b;}a++;b++;} return 0; }
char *kstrcpy(char *dst, char *src){while(*src) {*dst++=*src++;} *dst=0; return dst; }
/* shuffle bits of random seed */
void kentropy() { srand[0]*=16807; }
/* kernel panic */
extern void kpanic(char *reason, ...) {
    kprintf_fade();
    kprintf_fg(0xFFDD33); kprintf_bg(0);
    kprintf(kpanicprefix);
    kprintf("%s",reason);
#if DEBUG
    dbg_putchar(13);
    dbg_putchar(10);
#endif
    kprintf_center(54, 5);
    kprintf_fg(0x9c3c1b); kprintf_bg(0x100000);
    kprintf("%s",kpanicsuffix);
    kprintf_fg(0x500000);
    kprintf("%s",kpanicsuffix2);
    kx+=46; ky-=7;
    kprintf_putlogo();
    platform_waitkey();
    kprintf_reboot();
}
