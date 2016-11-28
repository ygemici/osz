/*
 * core.c
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
 * @brief Core
 *
 *   Memory map
 *       -512M framebuffer                   (0xFFFFFFFFE0000000)
 *       -2M core        bootboot struct     (0xFFFFFFFFFFE00000)
 *           -2M+1page   environment         (0xFFFFFFFFFFE01000)
 *           -2M+2page.. core text segment v (0xFFFFFFFFFFE02000)
 *            ..0        core stack        ^
 *       0-16G RAM identity mapped
 */

#include "core.h"

/**********************************************************************
 *                         OS/Z Core start                            *
 **********************************************************************
*/
void main()
{
	// initialize kernel version of printf
	kprintf_init();
#if DEBUG
	kprintf("OS/Z Starting...");
#endif
	// initialize physical memory manager
	pmm_init();

	int x,y,s=bootboot.fb_scanline,w=bootboot.fb_width,h=bootboot.fb_height;
	// cross-hair
	for(y=0;y<h;y++) { *((uint32_t*)(&fb + s*y + (w*2)))=0x00FFFFFF; }
	for(x=0;x<w;x++) { *((uint32_t*)(&fb + s*(h/2)+x*4))=0x00FFFFFF; }
	
	// boxes
	for(y=0;y<16;y++) { for(x=0;x<16;x++) { *((uint32_t*)(&fb + s*(y+16) + (x+10)*4))=0x00FF0000; } }
	for(y=0;y<16;y++) { for(x=0;x<16;x++) { *((uint32_t*)(&fb + s*(y+16) + (x+40)*4))=0x0000FF00; } }
	for(y=0;y<16;y++) { for(x=0;x<16;x++) { *((uint32_t*)(&fb + s*(y+16) + (x+70)*4))=0x000000FF; } }
    
	kprintf("\n\n  R   G   B\nHello OSDev!   "
		"%%d, %%c, %%x, '%%s' efiptr=%%x   %d, %c, %x, '%s' efiptr=%x\n"
		"utf-8 0-7FF: ¾ßŔƂ ΑΒΓΔΕΖΗΘ αβγδεζηθ ϢϣϤϥϦ ЀЁЂЃЄАБВГДЕЖ абвгдеж ЉԱԲԳԴ אבגד ابةتثج ۱۲۳ ܐܒܓܔܕ \n  ",
		65,'B',0x1a2b3c4d5e6f,"s%domething", bootboot.efi_ptr);

	__asm__ __volatile__ ( "movq %0, %%rax;xchgw %%bx,%%bx;cli;hlt" : : "m"(bootboot.efi_ptr) : "memory" );

}
