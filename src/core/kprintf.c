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
 * @brief Ring 0 printf implementation for core, early console
 */

#include <arch.h>
#include "font.h"

/* re-entrant counter, temp string */
char reent, tmpstr[33];
/* first position in line, current cursor, maximum coordinates, scrolled */
int fx, kx, ky, maxx, maxy, scry;

/* colors */
uint32_t fg, bg;

/* argument counter */
uint8_t cnt;

/* string constants and ascii arts */
char kpanicprefix[] = "OS/Z panic: ";
char kpanicrip[] = " @%x ";
char kpanicsym[] = "%s  ";
char kpanictlb[] = "paging error resolving %8x at step %d. ";
char kpanicsuffix[] =
    "                                                      \n"
    "   __|  _ \\  _ \\ __|   _ \\  \\    \\ |_ _|  __|         \n"
    "  (    (   |   / _|    __/ _ \\  .  |  |  (            \n"
    " \\___|\\___/ _|_\\___|  _| _/  _\\_|\\_|___|\\___|         \n";
char __attribute__((aligned(16))) kpanicsuffix2[] =
    "                                                      \n"
    "                       KEEP CALM                      \n"
    "               AND RESTART YOUR COMPUTER              \n"
    "                                                      \n";
char rebootprefix[] =
    "OS/Z rebooting...\n";
char poweroffprefix[] =
    "OS/Z shutdown finished.\n";
char poweroffsuffix[] =
    "TURN OFF YOUR COMPUTER.\n";
char nullstr[] = "(null)";

extern char _binary_logo_start;
extern uint64_t lastfps;
#if DEBUG
extern uint8_t dbg_indump;
extern uint8_t dbg_tui;
extern void dbg_setpos();
#endif

/**
 * reset console, white on black and move cursor home
 */
void kprintf_reset()
{
    kx = ky = fx = 0;
    scry = maxy - 1;
    reent = 0;
    fg = 0xC0C0C0;
    bg = 0;
}

/**
 * set foreground color
 */
void kprintf_fg(uint32_t color)
{
    switch(bootboot.fb_type) {
        case FB_ABGR: fg=((color>>16)&0xff)|(color&0xff00)|((color&0xff)<<16); break;
        default: fg=color; break;
    }
}

/**
 * set background color
 */
void kprintf_bg(uint32_t color)
{
    switch(bootboot.fb_type) {
        case FB_ABGR: bg=((color>>16)&0xff)|(color&0xff00)|((color&0xff)<<16); break;
        default: bg=color; break;
    }
}

/**
 * move cursor to top left corner of a centered window
 */
void kprintf_center(int w, int h)
{
    kx = fx = (maxx-w)/2;
    ky = (maxy-h)/2-1;
}

/**
 * initialize early console
 */
void kprintf_init()
{
    int x, y, line, offs = 0;
    font_t *font = (font_t*)&_binary_font_start;
    // clear screen
    for(y=0;y<bootboot.fb_height;y++){
        line=offs;
        for(x=0;x<bootboot.fb_width/2;x++){
            *((uint64_t*)(FBUF_ADDRESS + line))=(uint64_t)0;
            line+=8;
        }
        offs+=bootboot.fb_scanline;
    }
    // get console dimensions
    maxx = bootboot.fb_width / (font->width+1);
    maxy = bootboot.fb_height / font->height;
    // default fg and bg, cursor at home
    kprintf_reset();

    // display logo, shift it to darker blue
    char *data = &_binary_logo_start + 0x255;
    char *palette = &_binary_logo_start + 0x12;
    offs = ((bootboot.fb_height/2-32) * bootboot.fb_scanline) +
           ((bootboot.fb_width/2-32) * 4);
    for(y=0;y<64;y++){
        line=offs;
        for(x=0;x<64;x++){
            if(data[0]!=0) {
                // make it darker as normal
                kprintf_fg( (((uint8_t)palette[(uint8_t)data[0]*3+0]>>2)<<0)+
                            (((uint8_t)palette[(uint8_t)data[0]*3+1]>>2)<<8)+
                            (((uint8_t)palette[(uint8_t)data[0]*3+2]>>2)<<16) );
                *((uint32_t*)(FBUF_ADDRESS + line))=fg;
            }
            data++;
            line+=4;
        }
        offs+=bootboot.fb_scanline;
    }
    fg = 0xC0C0C0;
}

/**
 *  display Good Bye screen and reboot computer
 */
void kprintf_reboot()
{
    // say we're rebooting (on serial too)
    kprintf_init();
#if DEBUG
    dbg_putchar('\n');
#endif
    kprintf(rebootprefix);
    // reboot computer
    platform_reset();
    // if it didn't work, show a message and freeze.
    fg = 0x29283f;
    kprintf_center(20, -8);
    kprintf(poweroffsuffix);
#if DEBUG
    dbg_putchar('\n');
#endif
    platform_halt();
}

/**
 * display Good Bye screen and turn off computer
 */
void kprintf_poweroff()
{
    // say we're finished (on serial too)
    kprintf_init();
#if DEBUG
    dbg_putchar('\n');
#endif
    kprintf(poweroffprefix);
    // turn off hardware
    platform_poweroff();

    // if it didn't work, show a message and freeze.
    fg = 0x29283f;
    kprintf_center(20, -8);
    kprintf(poweroffsuffix);
#if DEBUG
    dbg_putchar('\n');
#endif
    platform_halt();
}

/**
 * put logo on panic screen, colours shifted to red
 */
void kprintf_putlogo()
{
    font_t *font = (font_t*)&_binary_font_start;
    int offs =
        ((ky * font->height - font->height/2) * bootboot.fb_scanline) +
        (kx * (font->width+1) * 4);
    int x,y, line;
    char *data = &_binary_logo_start + 0x255;
    for(y=0;y<64;y++){
        line=offs;
        for(x=0;x<64;x++){
            if(data[0]!=0) {
                // make it red instead of blue
                kprintf_fg( (((uint8_t)data[2]>>4)<<0)+
                            (((uint8_t)data[1]>>2)<<8)+
                            (((uint8_t)data[0]>>0)<<16) );
                *((uint32_t*)(FBUF_ADDRESS + line))=fg;
            }
            data++;
            line+=4;
        }
        offs+=bootboot.fb_scanline;
    }
    fg = 0xC0C0C0;
}

/**
 * display one literal unicode character
 */
void kprintf_putchar(int c)
{
    font_t *font = (font_t*)&_binary_font_start;
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
            *((uint32_t*)(FBUF_ADDRESS + line))=((int)*glyph) & (mask)?fg:bg;
            mask>>=1;
            line+=4;
        }
        *((uint32_t*)(FBUF_ADDRESS + line))=bg;
        glyph+=bytesperline;
        offs+=bootboot.fb_scanline;
    }
    srand[(offs+c)%4] += (uint64_t)c;
    srand[(offs+c+1)%4] -= ticks[TICKS_LO];
    srand[(c+bootboot.datetime[7]+bootboot.datetime[6]+bootboot.datetime[5])%4] *= 16807;
#if DEBUG
    // send it to serial too
    dbg_putchar(c);
#endif
}

/**
 * display ascii variable for %a
 */
void kprintf_putascii(int64_t c)
{
    *((uint64_t*)&tmpstr) = c;
    tmpstr[cnt>0&&cnt<=8?cnt:8]=0;
    kprintf(&tmpstr[0]);
}

/**
 * similar, for %A, mask out control chars
 */
void kprintf_dumpascii(int64_t c)
{
    int i;
    *((uint64_t*)&tmpstr) = c;
    if(cnt<1||cnt>8)
        cnt = 8;
    for(i=0;i<cnt;i++)
        if((uchar)tmpstr[i]==0 ||
            ((uchar)tmpstr[i]<' ' && (uchar)tmpstr[i]!=27) ||
            (uchar)tmpstr[i]>=127)
            tmpstr[i]='.';
    tmpstr[cnt]=0;
    kprintf(&tmpstr[0]);
}

/**
 * handle %d
 */
void kprintf_putdec(int64_t c)
{
    int64_t i=18,s=c<0;
    if(s) c*=-1;
    if(c>99999999999999999LL)
        c=99999999999999999LL;
    tmpstr[i]=0;
    do {
        tmpstr[--i]='0'+(c%10);
        c/=10;
    } while(c!=0&&i>0);
    if(s)
        tmpstr[--i]='-';
    if(cnt>0&&cnt<18) {
        while(i>18-cnt) {
            tmpstr[--i]=' ';
        }
    }
    kprintf(&tmpstr[i]);
}

/**
 * for %x arguments
 */
void kprintf_puthex(int64_t c)
{
    int i=16;
    tmpstr[i]=0;
    do {
        char n=c & 0xf;
        tmpstr[--i]=n<10?'0'+n:'A'+n-10;
        c>>=4;
    } while(c!=0&&i>0);
    if(cnt>0&&cnt<=8) {
        while(i>16-cnt*2) {
            tmpstr[--i]='0';
        }
    }
    kprintf(&tmpstr[i]);
}

/**
 * dump memory, for %D
 */
void kprintf_dumpmem(uint8_t *mem)
{
    uint8_t *ptr;
    int i,j,k=cnt;
#if DEBUG
    int old=dbg_indump;
#endif
    if(k<1 || k>128) k=16;
    kprintf("\n");
    for(j=0;j<k;j++) {
        cnt=4;
        kprintf_puthex((int64_t)mem);
        kprintf_putchar(':');kx++;
        kprintf_putchar(' ');kx++;
        ptr=mem; cnt=1;
        for(i=0;i<16;i++) {
            kprintf_puthex(*ptr);
            kprintf_putchar(' ');kx++;
            if(i%4==3) {
                kprintf_putchar(' ');kx++;
            }
            ptr++;
        }
        ptr=mem;
#if DEBUG
        dbg_indump=2;
#endif
        for(i=0;i<16;i++) {
            kprintf_putchar(*ptr);kx++;
            ptr++;
        }
#if DEBUG
        dbg_indump=old;
#endif
        mem+=16;
        kprintf("\n");
    }
}

/**
 * for %U, uuid
 */
void kprintf_uuid(uuid_t *mem)
{
    int i;
    cnt=4;
    kprintf_puthex((uint32_t)mem->Data1);
    kprintf_putchar('-'); kx++;
    cnt=2;
    kprintf_puthex((uint16_t)mem->Data2);
    kprintf_putchar('-'); kx++;
    kprintf_puthex((uint16_t)mem->Data3);
    kprintf_putchar('-'); kx++;
    cnt=1;
    for(i=0;i<8;i++)
        kprintf_puthex(mem->Data4[i]);
}

/**
 * called by isr_timer every sec, for debug
 */
void kprintf_putfps()
{
    int ox=kx,oy=ky,of=fg,ob=bg;
    kx=0; ky=maxy-1; bg=0;
    fg=0x801010;
    kprintf(" %d fps, ts %d",lastfps,ticks[TICKS_TS]);
#if DEBUG
    dbg_putchar('\n');
#endif
    kx=ox; ky=oy;
    fg=of; bg=ob;
}

/**
 * scroll the screen, and pause if srcy is set
 */
void kprintf_scrollscr()
{
    font_t *font = (font_t*)&_binary_font_start;
    int offs = 0, tmp = (maxy-1)*font->height*bootboot.fb_scanline;
    int x,y, line, shift=font->height*bootboot.fb_scanline;
    int w = maxx*(font->width+1)/2;
    // pause
    if(scry!=-1) {
        scry++;
        // if we've scrolled the whole screen, stop
        if(scry>=maxy-2) {
            scry = 0;
            uint32_t oldfg = fg;
            fg = 0xffffff;
            // display a message
            char *msg = " --- Press any key to continue --- ";
            kx = 0; ky=maxy-1;
#if DEBUG
        dbg_putchar('\n');
#endif
            for(;*msg!=0;msg++) {
                    kprintf_putchar((int)(*msg));
                    kx++;
            }
            // restore color
            fg = oldfg;
            kx = fx; ky--;
            // wait for user input. Turn off pause if user presses Esc
            if(platform_waitkey()==1)
                scry=-1;
            // clear the last row
            for(y=0;y<font->height;y++){
                line=tmp;
                for(x=0;x<w;x++){
                    *((uint64_t*)(FBUF_ADDRESS + line)) = (uint64_t)0;
                    line+=8;
                }
                tmp+=bootboot.fb_scanline;
            }
        }
        // scroll the screen, two pixels at once
        for(y=0;y<maxy*font->height;y++){
            line=offs;
            for(x=0;x<w;x++){
                *((uint64_t*)(FBUF_ADDRESS + line)) =
                    *((uint64_t*)(FBUF_ADDRESS + line + shift));
                line+=8;
            }
            offs+=bootboot.fb_scanline;
        }
    } else {
        ky = 0;
    }
}

/**
 * fade the screen
 */
void kprintf_fade()
{
    font_t *font = (font_t*)&_binary_font_start;
    int offs = 0;
    int x,y, line;
    int w = maxx*(font->width+1)/2;
    kprintf_reset();
    // darken the screen, two pixels at once
    for(y=0;y<maxy*font->height;y++){
        line=offs;
        for(x=0;x<w;x++){
            *((uint64_t*)(FBUF_ADDRESS + line)) =
                (*((uint64_t*)(FBUF_ADDRESS + line))>>2)&0x3F3F3F3F3F3F3F3FULL;
            line+=8;
        }
        offs+=bootboot.fb_scanline;
    }
}

/**
 * clear current line
 */
void kprintf_clearline()
{
    font_t *font = (font_t*)&_binary_font_start;
    int x,y, line, tmp = ky*font->height*bootboot.fb_scanline;
#if DEBUG
    if(dbg_indump)
        return;
#endif
    // clear the row
    for(y=0;y<font->height;y++){
        line=tmp;
        for(x=0;x<bootboot.fb_width;x+=2){
            *((uint64_t*)(FBUF_ADDRESS + line)) = (uint64_t)0;
            line+=8;
        }
        tmp+=bootboot.fb_scanline;
    }
}

#if DEBUG
/**
 * for testing purposes, print out all characters
 */
void kprintf_unicodetable()
{
    int x,y;
    // UNICODE table
    for(y=0;y<33;y++){
        kx=fx;
        for(x=0;x<64;x++){
            kprintf_putchar(y*64+x);
            kx++;
        }
        kprintf("\n");
    }
}
#endif

/**
 * display a string on early console
 */
void vkprintf(char *fmt, va_list args)
{
    uint64_t arg;
    char *p;

    if(fmt==NULL)
        fmt=nullstr;
#if DEBUG
    /* starts with special (esc)[UT CSI command? */
    if(*((uint32_t*)fmt)==(uint32_t)0x54555B1B) {
        /* print out the whole charset */
        kprintf_unicodetable();
        return;
    }
#endif
    arg = 0;
    while(fmt[0]!=0) {
        // special characters
        if(fmt[0]==8) {
            // backspace
            kx--;
#if DEBUG
            dbg_putchar(8);
#endif
            kprintf_putchar((int)' ');
        } else
        if(fmt[0]==9) {
            // tab
            kx=((kx+8)/8)*8;
#if DEBUG
            dbg_putchar(9);
#endif
        } else
        if(fmt[0]==10) {
            // newline
            goto newline;
        } else
        if(fmt[0]==13) {
            // carridge return
            kx=fx;
#if DEBUG
            dbg_putchar(13);
#endif
        } else
        // argument access
        if(fmt[0]=='%' && !reent) {
            fmt++; cnt=0;
            if(fmt[0]=='%') {
                goto put;
            }
            while(fmt[0]>='0'&&fmt[0]<='9') {
                cnt *= 10;
                cnt += fmt[0]-'0';
                fmt++;
            }
            if(fmt[0]!='s')
                arg = va_arg(args, int64_t);
            if(fmt[0]=='c') {
                kprintf_putchar((int)((unsigned char)arg));
                goto nextchar;
            }
            reent++;
            if(fmt[0]=='a') {
                kprintf_putascii(arg);
            } else
            if(fmt[0]=='A') {
                kprintf_dumpascii(arg);
            } else
            if(fmt[0]=='D') {
                kprintf_dumpmem((uint8_t*)arg);
            } else
            if(fmt[0]=='d') {
                kprintf_putdec(arg);
            } else
            if(fmt[0]=='x') {
                kprintf_puthex(arg);
            } else
            if(fmt[0]=='U') {
                kprintf_uuid((uuid_t*)arg);
            } else
            if(fmt[0]=='s') {
                p = va_arg(args, char*);
                kprintf(p);
                // padding with spaces
                arg=kstrlen(p); if(arg<cnt) { cnt-=arg; while(cnt-->0) { kprintf_putchar(' '); kx++; } }
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
#if DEBUG
                    if(dbg_indump)
                        return;
#endif
                    kprintf_scrollscr();
                }
#if DEBUG
                if(dbg_tui)
                    dbg_setpos();
                else
                    dbg_putchar('\n');
                if(scry==-1 && !dbg_indump)
#else
                if(scry==-1)
#endif
                    kprintf_clearline();
            }
        }
        fmt++;
    }
}

void kprintf(char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    return vkprintf(fmt,args);
}
