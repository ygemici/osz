/*
 * ui/keys.c
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
 * @brief User Interface key routines
 */
#include <osZ.h>

// key modifiers
#define KEYFLAG_SHIFT (1<<0)
#define KEYFLAG_CTRL  (1<<1)
#define KEYFLAG_ALT   (1<<2)
#define KEYFLAG_SUPER (1<<3)
// index to keymap
uint8_t modmap[16] = {0,
    1/*shft*/, 2/*ctrl*/, 4/*alt*/, 8/*super*/,
    3/*shft+ctrl*/, 5/*shft+alt*/, 9/*shft+super*/,
    6/*ctrl+alt*/, 10/*ctrl+super*/,
    12/*alt+super*/,
    7/*shft+ctrl+alt*/,11/*shft+ctrl+super*/,13/*shft+alt+super*/,14/*ctrl+alt+super*/,
    15/*shft+ctrl+alt+super*/
};
// default or alternate map
uint8_t altmap = 0;
// current modifier status
uint8_t keyflags = 0;
// a three dimensional array of keycodes
// 0: indexed by altmap (scalar)
// 1: scancode
// 2: modifier keys indexed by modmap (array index)
keymap_t keymap[2*512*16];

/* parse an UTF-8 text/plain keymap resource file into binary format */
private void keymap_parse(bool_t alt, char *keyrc, size_t len)
{
    // local buffer pointers
    // current and end of buffer
    char *c = keyrc, *e = keyrc + len;
    // variables
    uint64_t scancode, i;
    // temporary buffer for character reference
    uint8_t key[4];

    while(c<e && *c!=0) {
        // skip comments
        if(*c=='#' || (*c=='/'&&*(c+1)=='/')) {
            c++;
            while(c<e && *c!=0 && *(c-1)!='\n') c++;
        }
        if((*c=='/'&&*(c+1)=='*')) {
            c+=2;
            while(c<e && *c!=0 && *(c-2)!='*' && *(c-1)!='/') c++;
        }
        // skip empty lines
        while(c<e && (*c=='\n'||*c==' ')) c++;
        // get scancode
        c = (char*)stdlib_dec((unsigned char *)c, &scancode, 0, 511); c++;
        // load key symbols
        i=0; *((uint32_t*)&key)=0;
        while(c<e && *c!='\n' && *c!=0) {
            // key keycode. Up to 4 bytes, zero padded
            // UTF-8 multichar or more ASCII bytes (control)
            while(c<e && *c==' ') c++;
            if(scancode && i<16) {
                key[0] = (uint8_t)(*c); c++;
                if(*c!=' ' && *c!='\n') {
                    key[1] = (uint8_t)(*c); c++;
                    if(*c!=' ' && *c!='\n') {
                        key[2] = (uint8_t)(*c); c++;
                        if(*c!=' ' && *c!='\n') {
                            key[3] = (uint8_t)(*c);
                        }
                    }
                }
                // save into keyboard map
                keymap[alt*(512*16)+scancode*16+i] = *((keymap_t*)&key[0]);
                // failsafe
                if(*c==0||*c=='\n')
                    continue;
                // get next modified version
                i++;
            }
            c++;
        }
    }
}

/* receive a scancode or a pretranslated keycode */
public void keypress(uint64_t scancode, uint32_t keycode)
{
    uint8_t k[4] = { 0, 0, 0, 0 };
    uint64_t i = altmap*(512*16)+scancode*16;
    // look up scancode
    if(scancode!=0) {
        // messages sent by keyboard comes with scancode
        *((keymap_t*)&k) = keymap[i+modmap[keyflags]] ?
            keymap[i+modmap[keyflags]] : keymap[i];
    } else {
        // serial on the other hand transmits keycode and CSI multibytes
        *((keymap_t*)&k) = keycode;
    }
    // handle key modifiers
    if(k[0]=='L' || k[0]=='R') {
        if(k[1]=='S' && k[2]=='f' && k[3]=='t')
            keyflags |= KEYFLAG_SHIFT;
        if(k[1]=='C' && k[2]=='t' && k[3]=='l')
            keyflags |= KEYFLAG_CTRL;
        if(k[1]=='A' && k[2]=='l' && k[3]=='t')
            keyflags |= KEYFLAG_ALT;
        if(k[1]=='S' && k[2]=='p' && k[3]=='r')
            keyflags |= KEYFLAG_SUPER;
    }
#if DEBUG
    dbg_printf("keypress %d %c%c%c%c\n", scancode, k[0], k[1], k[2], k[3]);
#endif
//asm("movl %0, %%eax;movq %1, %%rbx;xchg %%bx,%%bx;int $1"::"r"(*((uint32_t*)&k)),"r"(scancode):);
}

public void keyrelease(uint64_t scancode)
{
    uint8_t k[4] = { 0, 0, 0, 0 };
    uint64_t i = altmap*(512*16)+scancode*16;

    // look up scancode
    if(scancode!=0) {
        // messages sent by keyboard comes with scancode
        *((keymap_t*)&k) = keymap[i+modmap[keyflags]] ?
            keymap[i+modmap[keyflags]] : keymap[i];
        // serial sends no release messages
    }
    // handle key modifiers
    if(k[0]=='L' || k[0]=='R') {
        if(k[1]=='S' && k[2]=='f' && k[3]=='t')
            keyflags &= ~KEYFLAG_SHIFT;
        if(k[1]=='C' && k[2]=='t' && k[3]=='l')
            keyflags &= ~KEYFLAG_CTRL;
        if(k[1]=='A' && k[2]=='l' && k[3]=='t')
            keyflags &= ~KEYFLAG_ALT;
        if(k[1]=='S' && k[2]=='p' && k[3]=='r')
            keyflags &= ~KEYFLAG_SUPER;
    }
#if DEBUG
    dbg_printf("keyrelease %d %c%c%c%c\n", scancode, k[0], k[1], k[2], k[3]);
#endif
}
