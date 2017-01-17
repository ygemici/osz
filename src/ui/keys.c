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

extern char *stdlib_dec(char *s, uint64_t *v, uint64_t min, uint64_t max);

#define KEYFLAG_SHIFT (1<<0)
#define KEYFLAG_CTRL  (1<<1)
#define KEYFLAG_ALT   (1<<2)
#define KEYFLAG_SUPER (1<<3)
uint8_t modmap[16] = {0,
    1/*shft*/, 2/*ctrl*/, 4/*alt*/, 8/*super*/,
    3/*shft+ctrl*/, 5/*shft+alt*/, 9/*shft+super*/,
    6/*ctrl+alt*/, 10/*ctrl+super*/,
    12/*alt+super*/,
    7/*shft+ctrl+alt*/,11/*shft+ctrl+super*/,13/*shft+alt+super*/,14/*ctrl+alt+super*/,
    15/*shft+ctrl+alt+super*/
};

uint8_t altmap = 0;
uint8_t keyflags = 0;
uint32_t keymap[2*512*16];

private void keymap_parse(bool_t alt, char *keyrc, size_t len)
{
    char *c = keyrc, *e = keyrc + len;
    uint64_t scancode, i;
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
        while(c<e && (*c=='\n'||*c==' ')) c++;
        // get scancode
        c = stdlib_dec(c, &scancode, 0, 511); c++;
        // load key symbols
        i=0; *((uint32_t*)&key)=0;
        while(c<e && *c!='\n' && *c!=0) {
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
                keymap[alt*(512*16)+scancode*16+i] = *((uint32_t*)&key[0]);
                if(*c==0||*c=='\n')
                    continue;
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
    if(scancode!=0) {
        *((uint32_t*)&k) = keymap[i+modmap[keyflags]] ?
            keymap[i+modmap[keyflags]] : keymap[i];
    } else {
        *((uint32_t*)&k) = keycode;
    }
asm("movl %0, %%eax;movq %1, %%rbx;xchg %%bx,%%bx;int $1"::"r"(*((uint32_t*)&k)),"r"(scancode):);
}

public void keyrelease(uint64_t scancode)
{

}
