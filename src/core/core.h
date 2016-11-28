/*
 * core.h
 * 
 * Copyright 2016 CC-by-nc-sa bztsrc@github
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
 * @brief Core functions (ring 0)
 */

#include <osZ.h>
#include <limits.h>
#include "../../loader/bootboot.h"

// import virtual addresses from linker
extern BOOTBOOT bootboot;
extern unsigned char environment[4096];
extern uint8_t fb;

// kernel function routines
extern void kprintf_init();
extern void env_init();
extern void pmm_init();
extern void* kmap_init();
extern void kprintf(char* ptr, ...);
extern void kmemcpy(char *dest, char *src, int size);
extern void kmemset(char *dest, int c, int size);
extern int kmemcmp(void *dest, void *src, int size);
extern void kmemvid(char *dest, char *src, int size);
extern void* kalloc(int pages);
extern void kfree(void* ptr);
extern void kmap(uint64_t virt, uint64_t phys);
