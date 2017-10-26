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
extern void vkprintf(char *fmt, va_list args);
extern phy_t tmppte, identity_mapping, _data;
extern uint64_t *kmap_tmp;

int kstrlen(char *s) { uint32_t n=0; while(*s++) n++; return n; }
void kmemcpy(void *dst, void *src, int n){uint8_t *a=dst,*b=src;while(n--) *a++=*b++; }
void kmemset(void *dst, int c, int n){uint8_t *a=dst;while(n--) *a++=c; }
int kmemcmp(void *s1, void *s2, int n){uint8_t *a=s1,*b=s2;while(n--){if(*a!=*b){return *a-*b;}a++;b++;} return 0; }
char *kstrcpy(char *dst, char *src){while(*src) {*dst++=*src++;} *dst=0; return dst; }
/* shuffle bits of random seed */
void kentropy() {
    srand[(uint8_t)(srand[0]>>16)%4]*=16807;
    srand[(uint8_t)srand[0]%4]<<=1; srand[(uint8_t)(srand[0]>>8)%4]>>=1;
    srand[(uint8_t)srand[1]%4]^=srand[((uint8_t)srand[2])%4];
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

uint64_t *kmap_init()
{
    uint64_t *ptr, l2, l3;
    uint32_t i,j;
    asm volatile ("mrs %0, ttbr0_el1" : "=r" (identity_mapping));
    asm volatile ("mrs %0, ttbr1_el1" : "=r" (l2));
    l3= (0xFF << 0) |    // Attr=0: normal, IWBWA, OWBWA, NTR
        (0x04 << 8) |    // Attr=1: device, nGnRE (must be OSH too)
        (0x44 <<16);     // Attr=2: non cacheable
    asm volatile ("msr mair_el1, %0" : : "r" (l3));
    /* this is called very early. Relies on identity mapping
       to find the physical address of tmppte pointer in L3 */
    ptr=(uint64_t*)(l2&~0xFFF);
    // core L1
    ptr[511]&=~0xFFF; l2=ptr[511]; ptr[511]|=(1L<<61)|PG_CORE_NOCACHE|PG_PAGE; //APTable=1, no EL0 access
    // core L2
    ptr=(uint64_t*)l2;
    ptr[511]&=~0xFFF; l3=ptr[511]; ptr[511]|=PG_CORE_NOCACHE|PG_PAGE;
    // core L3
    ptr=(uint64_t*)l3;
    ptr[0]&=~0xFFF; ptr[0]|=(1L<<PG_NX_BIT)|PG_CORE|PG_PAGE;             // bootboot
    ptr[1]&=~0xFFF; ptr[1]|=(1L<<PG_NX_BIT)|PG_CORE|PG_PAGE;             // environment
    j=(uint32_t)(((uint64_t)&_data&0xfffff)>>12);
    for(i=2;i<j;i++) { ptr[i]&=~0xFFF; ptr[i]|=PG_CORE_RO|PG_PAGE; } // code
    for(i=j;ptr[i]&1;i++) { ptr[i]&=~0xFFF; ptr[i]|=(1L<<PG_NX_BIT)|PG_CORE_NOCACHE|PG_PAGE; } // data
    ptr[511]&=~0xFFF; ptr[511]|=(1L<<PG_NX_BIT)|PG_CORE_NOCACHE|PG_PAGE; // stack
    ptr[(((uint64_t)&tmpmqctrl)>>12)&0x1FF]=l2|(1L<<61)|PG_CORE_NOCACHE|PG_PAGE;
    ptr[(((uint64_t)&tmppte)>>12)&0x1FF]=l3|(1L<<61)|PG_CORE_NOCACHE|PG_PAGE;
    asm volatile ("dsb sy; tlbi vmalle1");
    asm volatile ("mrs %0, sctlr_el1" : "=r" (l3));
    l3|=(1<<19)|(1<<12)|(1<<2); // enable WXN, I instruction cache and C data cache
    asm volatile ("msr sctlr_el1, %0; isb" : : "r" (l3));
    return (void*)((uint64_t)&tmppte + (((((uint64_t)&tmpctrl)>>12)&0x1FF)<<3));
}

uint64_t *kmap_getpte2(virt_t addr, phy_t memroot)
{
    phy_t page;
    // failsafe
    if(addr>>48)
        asm volatile ("mrs %0, ttbr1_el1" : "=r" (memroot));
//kprintf("kmap_getpte %x %x\n",addr,memroot);
    *kmap_tmp=(memroot&0xFFFFFFFF000L)|PG_CORE_NOCACHE|PG_PAGE;
    asm volatile ("dsb ishst; tlbi vaale1, %0; dsb ish; isb" : : "r" ((virt_t)&tmpctrl>>12));
//asm volatile("at s1e1r, %1; mrs %1, par_el1":"=r"(page):"r"(&tmpctrl));
//kprintf("at s1e1r, &tmpctrl; par_el1 %x\n",page);
    page=(uint64_t)&tmpctrl|(((addr>>(12+9+9))&0x1FF)<<3);
    *kmap_tmp=(*((uint64_t*)page)&0xFFFFFFFF000L)|PG_CORE_NOCACHE|PG_PAGE;
    asm volatile ("dsb ishst; tlbi vaale1, %0; dsb ish; isb" : : "r" ((virt_t)&tmpctrl>>12));
//asm volatile("at s1e1r, %1; mrs %1, par_el1":"=r"(page):"r"(&tmpctrl));
//kprintf("at s1e1r, &tmpctrl; par_el1 %x\n",page);
    page=(uint64_t)&tmpctrl|(((addr>>(12+9))&0x1FF)<<3);
    *kmap_tmp=(*((uint64_t*)page)&0xFFFFFFFF000L)|PG_CORE_NOCACHE|PG_PAGE;
    asm volatile ("dsb ishst; tlbi vaale1, %0; dsb ish; isb" : : "r" ((virt_t)&tmpctrl>>12));
//asm volatile("at s1e1r, %1; mrs %1, par_el1":"=r"(page):"r"(&tmpctrl));
//kprintf("at s1e1r, &tmpctrl; par_el1 %x\n",page);
    page=(uint64_t)&tmpctrl|(((addr>>12)&0x1FF)<<3);
//kprintf("kmap_getpte return %x %x\n",*kmap_tmp,page);
    return (uint64_t*)page;
}

uint64_t *kmap_getpte(virt_t addr)
{
    uint64_t memroot;
    if(addr>>48)
        asm volatile ("mrs %0, ttbr1_el1" : "=r" (memroot));
    else
        asm volatile ("mrs %0, ttbr0_el1" : "=r" (memroot));
    return kmap_getpte2(addr,memroot);
}

void kmap(virt_t virt, phy_t phys, uint16_t access)
{
    uint64_t *ptr;
    // shortcuts for speed up
    if(virt==(virt_t)&tmpmap)   ptr=(uint64_t*)kmap_tmp[3]; else
    if(virt==(virt_t)&tmp2map)  ptr=(uint64_t*)kmap_tmp[4]; else
    if(virt==(virt_t)&tmpalarm) ptr=(uint64_t*)kmap_tmp[5]; else
    if(virt==(virt_t)&tmpfx)    ptr=(uint64_t*)kmap_tmp[6]; else
        ptr=kmap_getpte(virt);
    /* negate RO bit to NX bit */
    *ptr=phys|access|((access>>7)&1?0:(1L<<PG_NX_BIT));
    asm volatile ("dsb ishst; tlbi vaae1, %0; dc cvau, %1; dsb ish; isb" : : "r" (virt>>12), "r"(virt));
//kprintf("kmap(%x,%x,%x) %x=%x\n",virt, phys, access, ptr, *ptr);
}

void kmap_mq(phy_t tcbmemroot)
{
}

void kmap_buf(phy_t tcbmemroot, virt_t ptr, size_t size)
{
}
