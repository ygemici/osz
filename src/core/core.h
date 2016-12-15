/*
 * core/core.h
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
#include "pmm.h"

// import virtual addresses from linker
extern BOOTBOOT bootboot;
extern unsigned char environment[__PAGESIZE];
extern uint8_t fb;
extern uint8_t tmpmap;
extern uint8_t tmp2map;

// kernel variables
extern uint64_t fs_size;
extern uint64_t fullsize;

// kernel function routines
extern void kprintf_init();
extern void env_init();
extern void pmm_init();
extern void *pmm_alloc();
extern void* kmap_init();
extern void isr_init();
extern void fs_init();
extern void *fs_locate(char *fn);
extern void dev_init();
extern void kpanic(char *reason, ...);
extern void kprintf(char* fmt, ...);
extern void kmemcpy(char *dest, char *src, int size);
extern void kmemset(char *dest, int c, int size);
extern int kmemcmp(void *dest, void *src, int size);
extern void kmemvid(char *dest, char *src, int size);
extern void* kalloc(int pages);
extern void kfree(void* ptr);
extern void kmap(uint64_t virt, uint64_t phys, uint8_t access);
extern pid_t thread_new();
extern void *thread_loadelf(char *fn);
extern void thread_loadso(char *fn);
extern void thread_add(pid_t thread);
extern void thread_remove(pid_t thread);
extern void thread_dynlink(pid_t thread);
extern void thread_mapbss(uint64_t phys, uint64_t size);
extern void service_init(char *fn);
extern void service_init2(char *fn, uint64_t n, ...);
