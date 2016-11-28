/*
 * kprintf.c
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
 * @brief Ring 0 printf implementation for core
 */

#include "core.h"
#include "font.h"

/* first position in line */
int __attribute__ ((section (".data"))) fx;
/* current cursor position */
int __attribute__ ((section (".data"))) kx;
int __attribute__ ((section (".data"))) ky;
/* maximum coordinates */
int __attribute__ ((section (".data"))) maxx;
int __attribute__ ((section (".data"))) maxy;
/* colors */
uint32_t __attribute__ ((section (".data"))) fg;
uint32_t __attribute__ ((section (".data"))) bg;
/* re-entrant counter */
char __attribute__ ((section (".data"))) reent;
/* for temporary strings */
char __attribute__ ((section (".data"))) tmp[33];

typedef unsigned char *valist;
#define vastart(list, param) (list = (((valist)&param) + sizeof(void*)*6))
#define vaarg(list, type)    (*(type *)((list += sizeof(void*)) - sizeof(void*)))

void kprintf_init()
{
	OSZ_font *font = (OSZ_font*)&_binary_font_start;
	kx = ky = fx = 0;
	maxx = bootboot.fb_width / font->width;
	maxy = bootboot.fb_height / font->height;
	reent = 0;
	fg = 0xC0C0C0;
	bg = 0;
}

void kprintf_putchar(int c)
{
	OSZ_font *font = (OSZ_font*)&_binary_font_start;
	unsigned char *glyph = (unsigned char*)&_binary_font_start +
	 font->headersize +
	 (c>0&&c<font->numglyph?c:0)*font->bytesperglyph;
	int offs =
		(ky * font->height * bootboot.fb_scanline) +
		(kx * font->width * 4);
	int x,y, line,mask;
	int bytesperline=(font->width+7)/8;
	for(y=0;y<font->height;y++){
		line=offs;
		mask=1<<(font->width-1);
		for(x=0;x<font->width;x++){
			*((uint32_t*)(&fb + line))=((int)*glyph) & (mask)?fg:bg;
			mask>>=1;
			line+=4;
		}
		glyph+=bytesperline;
		offs+=bootboot.fb_scanline;
	}
}

void kprintf_putdec(int64_t c)
{
	int i=32;
	tmp[i]=0;
	do {
		tmp[--i]='0'+(c%10);
		c/=10;
	} while(c!=0&&i>0);
	kprintf(&tmp[i]);
}

void kprintf_puthex(int64_t c)
{
	int i=32;
	tmp[i]=0;
	do {
		char n=c & 0xf;
		tmp[--i]=n<10?'0'+n:'A'+n-10;
		c>>=4;
	} while(c!=0&&i>0);
	kprintf(&tmp[i]);
}

void kprintf(char* ptr, ...)
{
	valist args;
	uint64_t arg;
	char *p;
	vastart(args, ptr);
/*
// UNICODE table
int x,y;
for(y=0;y<32;y++){
	kx=0;ky++;
	for(x=0;x<64;x++){
		kprintf_putchar(y*64+x);
		kx++;
    }
}
*/
	while(ptr[0]!=0) {
		// special characters
		if(ptr[0]==8) {
			// backspace
			kx--;
			kprintf_putchar((int)' ');
		} else
		if(ptr[0]==9) {
			// tab
			kx=((kx+8)/8)*8;
		} else
		if(ptr[0]==10) {
			// newline
			goto newline;
		} else
		if(ptr[0]==13) {
			// carrige return
			kx=fx;
		} else
		// argument access
		if(ptr[0]=='%' && !reent) {
			ptr++;
			if(ptr[0]=='%') {
				goto put;
			}
			p = *((char**)(args));
			arg = vaarg(args, int64_t);
			if(ptr[0]=='c') {
				kprintf_putchar((int)((unsigned char)arg));
				goto nextchar;
			}
			reent++;
			if(ptr[0]=='d') {
				kprintf_putdec(arg);
			}
			if(ptr[0]=='x') {
				kprintf_puthex(arg);
			}
			if(ptr[0]=='s') {
				kprintf(p);
			}
			reent--;
		} else {
			// convert utf-8 (at ptr) into unicode (to arg)
put:		arg=(uint64_t)((unsigned char)ptr[0]);
			if((arg & 128) != 0) {
				if((arg & 32) == 0 ) {
					arg=((ptr[0] & 0x1F)<<6)+(ptr[1] & 0x3F);
					ptr++;
				} else
				if((arg & 16) == 0 ) {
					arg=((((ptr[0] & 0xF)<<6)+(ptr[1] & 0x3F))<<6)+(ptr[2] & 0x3F);
					ptr+=2;
				} else
				if((arg & 8) == 0 ) {
					arg=((((((ptr[0] & 0x7)<<6)+(ptr[1] & 0x3F))<<6)+(ptr[2] & 0x3F))<<6)+(ptr[3] & 0x3F);
					ptr+=3;
				} else
					arg=0;
			}
			// display unicode character and move cursor
			kprintf_putchar((int)arg);
nextchar:	kx++;
			if(kx>=maxx) {
newline:		kx=fx;
				ky++;
				if(ky>=maxy)
					ky=0;
			}
		}
		ptr++;
	}
}
