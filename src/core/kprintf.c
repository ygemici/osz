/*
 * core/kprintf.c
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

#include "font.h"

/* re-entrant counter */
char __attribute__ ((section (".data"))) reent;
/* for temporary strings */
char __attribute__ ((section (".data"))) tmp[33];
/* first position in line, used by carridge return */
int __attribute__ ((section (".data"))) fx;
/* current cursor position */
int __attribute__ ((section (".data"))) kx;
int __attribute__ ((section (".data"))) ky;
/* maximum coordinates */
int __attribute__ ((section (".data"))) maxx;
int __attribute__ ((section (".data"))) maxy;
/* scrolled lines counter */
int __attribute__ ((section (".data"))) scry;

/* colors */
uint32_t __attribute__ ((section (".data"))) fg;
uint32_t __attribute__ ((section (".data"))) bg;

typedef unsigned char *valist;
#define vastart(list, param) (list = (((valist)&param) + sizeof(void*)*6))
#define vaarg(list, type)    (*(type *)((list += sizeof(void*)) - sizeof(void*)))

extern char _binary_logo_tga_start;
extern void cpu_waitkey();

void kprintf_reset()
{
    kx = ky = fx = scry = 0;
    reent = 0;
    fg = 0xC0C0C0;
    bg = 0;
}

void kprintf_init()
{
    int x, y, line, offs = 0;
    OSZ_font *font = (OSZ_font*)&_binary_font_start;
    // clear screen
    for(y=0;y<bootboot.fb_height;y++){
        line=offs;
        for(x=0;x<bootboot.fb_width;x++){
            *((uint32_t*)(&fb + line))=(uint32_t)0;
            line+=4;
        }
        offs+=bootboot.fb_scanline;
    }
    // get console dimensions
    maxx = bootboot.fb_width / font->width;
    maxy = bootboot.fb_height / font->height;
    // default fg and bg, cursor at home
    kprintf_reset();
    scry = maxy - 2;
    // display boot logo
    char *data = &_binary_logo_tga_start + 0x255;
    char *palette = &_binary_logo_tga_start + 0x12;
    offs = ((bootboot.fb_height/2-32) * bootboot.fb_scanline) +
           ((bootboot.fb_width/2-32) * 4);
    for(y=0;y<64;y++){
        line=offs;
        for(x=0;x<64;x++){
            if(data[0]!=0) {
                // make it darker as normal
                *((uint32_t*)(&fb + line))=(uint32_t)(
                    (((uint8_t)palette[(uint8_t)data[0]*3+0]>>3)<<0)+
                    (((uint8_t)palette[(uint8_t)data[0]*3+1]>>3)<<8)+
                    (((uint8_t)palette[(uint8_t)data[0]*3+2]>>3)<<16)
                );
            }
            data++;
            line+=4;
        }
        offs+=bootboot.fb_scanline;
    }
}

void kprintf_putlogo()
{
    OSZ_font *font = (OSZ_font*)&_binary_font_start;
    int offs =
        ((ky * font->height - font->height/2) * bootboot.fb_scanline) +
        (kx * (font->width+1) * 4);
    int x,y, line;
    char *data = &_binary_logo_tga_start + 0x255;
    for(y=0;y<64;y++){
        line=offs;
        for(x=0;x<64;x++){
            if(data[0]!=0) {
                // make it red instead of blue
                *((uint32_t*)(&fb + line))=(uint32_t)(
                    (((uint8_t)data[2]>>4)<<0)+
                    (((uint8_t)data[1]>>2)<<8)+
                    (((uint8_t)data[0]>>0)<<16)
                );
            }
            data++;
            line+=4;
        }
        offs+=bootboot.fb_scanline;
    }
}

void kprintf_putchar(int c)
{
    OSZ_font *font = (OSZ_font*)&_binary_font_start;
    unsigned char *glyph = (unsigned char*)&_binary_font_start +
     font->headersize +
     (c>0&&c<font->numglyph?c:0)*font->bytesperglyph;
    int offs =
        (ky * font->height * bootboot.fb_scanline) +
        (kx * (font->width+1) * 4);
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
        *((uint32_t*)(&fb + line))=bg;
        glyph+=bytesperline;
        offs+=bootboot.fb_scanline;
    }
}

void kprintf_putascii(int64_t c)
{
	uint64_t *t = (uint64_t*)&tmp;
    int i;
    for(i=0;i<9;i++) tmp[i]=0;
	*t = c;
    kprintf(&tmp[0]);
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
    int i=16;
    tmp[i]=0;
    do {
        char n=c & 0xf;
        tmp[--i]=n<10?'0'+n:'A'+n-10;
        c>>=4;
    } while(c!=0&&i>0);
    kprintf(&tmp[i]);
}

void kprintf_scrollscr()
{
    OSZ_font *font = (OSZ_font*)&_binary_font_start;
    int offs = 0, tmp;
    int x,y, line, shift=font->height*bootboot.fb_scanline;
    for(y=0;y<(maxy-1)*font->height;y++){
        line=offs;
        for(x=0;x<maxx*(font->width+1);x++){
            *((uint32_t*)(&fb + line)) =
                *((uint32_t*)(&fb + line + shift));
            line+=4;
        }
        offs+=bootboot.fb_scanline;
    }
    // clear the last row
    tmp = offs;
    for(y=0;y<font->height;y++){
        line=offs;
        for(x=0;x<maxx*(font->width+1);x++){
            *((uint32_t*)(&fb + line)) = (uint32_t)0;
            line+=4;
        }
        offs+=bootboot.fb_scanline;
    }
    scry++;
    // if we've scrolled the whole screen, stop
    if(scry>=maxy-1) {
        uint32_t oldfg = fg;
        fg = 0xffffff;
        // display a message
        char *msg = " --- Press any key to continue --- ";
        scry = kx = 0;
        for(;*msg!=0;msg++) {
                kprintf_putchar((int)(*msg));
                kx++;
        }
        // wait for user input
        cpu_waitkey();
        // clear the message
        for(y=0;y<font->height;y++){
            line=tmp;
            for(x=0;x<maxx*(font->width+1);x++){
                *((uint32_t*)(&fb + line)) = (uint32_t)0;
                line+=4;
            }
            tmp+=bootboot.fb_scanline;
        }
        // restore color
        fg = oldfg;
        kx = fx;
    }
}

// for testing purposes
void kprintf_unicodetable()
{
    int x,y;
    kprintf_reset();
    // UNICODE table
    for(y=0;y<33;y++){
        kx=0;ky++;
        for(x=0;x<64;x++){
            kprintf_putchar(y*64+x);
            kx++;
        }
    }
}

void kprintf(char* fmt, ...)
{
    valist args;
    vastart(args, fmt);
    uint64_t arg;
    char *p;

// print out the whole charset
// return kprintf_unicodetable();

    while(fmt[0]!=0) {
        // special characters
        if(fmt[0]==8) {
            // backspace
            kx--;
            kprintf_putchar((int)' ');
        } else
        if(fmt[0]==9) {
            // tab
            kx=((kx+8)/8)*8;
        } else
        if(fmt[0]==10) {
            // newline
            goto newline;
        } else
        if(fmt[0]==13) {
            // carridge return
            kx=fx;
        } else
        // argument access
        if(fmt[0]=='%' && !reent) {
            fmt++;
            if(fmt[0]=='%') {
                goto put;
            }
            p = *((char**)(args));
            arg = vaarg(args, int64_t);
            if(fmt[0]=='c') {
                kprintf_putchar((int)((unsigned char)arg));
                goto nextchar;
            }
            reent++;
            if(fmt[0]=='a') {
                kprintf_putascii(arg);
            }
            if(fmt[0]=='d') {
                kprintf_putdec(arg);
            }
            if(fmt[0]=='x') {
                kprintf_puthex(arg);
            }
            if(fmt[0]=='s') {
                kprintf(p);
            }
            reent--;
        } else {
            // convert utf-8 (at fmt) into unicode (to arg)
put:        arg=(uint64_t)((unsigned char)fmt[0]);
            if((arg & 128) != 0) {
                if((arg & 32) == 0 ) {
                    arg=((fmt[0] & 0x1F)<<6)+(fmt[1] & 0x3F);
                    fmt++;
                } else
                if((arg & 16) == 0 ) {
                    arg=((((fmt[0] & 0xF)<<6)+(fmt[1] & 0x3F))<<6)+(fmt[2] & 0x3F);
                    fmt+=2;
                } else
                if((arg & 8) == 0 ) {
                    arg=((((((fmt[0] & 0x7)<<6)+(fmt[1] & 0x3F))<<6)+(fmt[2] & 0x3F))<<6)+(fmt[3] & 0x3F);
                    fmt+=3;
                } else
                    arg=0;
            }
            // display unicode character and move cursor
            kprintf_putchar((int)arg);
nextchar:   kx++;
            if(kx>=maxx) {
newline:        kx=fx;
                ky++;
                if(ky>=maxy) {
                    ky--;
                    kprintf_scrollscr();
                }
            }
        }
        fmt++;
    }
}
