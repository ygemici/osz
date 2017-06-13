/*
 * ui/pointer.c
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
 * @brief User Interface pointer routines
 */
#include <osZ.h>
#include <sys/video.h>

extern uint64_t _fb_width;
extern uint64_t _fb_height;

// current modifier status
extern uint8_t keyflags;

// pointer status
typedef struct {
    uint32_t btn;
    uint32_t x;
    uint32_t y;
    uint32_t z;
} pointer_t;

pointer_t pointers[2];

void reset_pointers()
{
    pointers[0].btn = 0;
    pointers[0].x = _fb_width/2;
    pointers[0].y = _fb_height/2;
    pointers[0].z = 0;
//    vid_movecursor(0,pointers[0].x,pointers[0].y);
    vid_setcursor(0,VC_text);
    pointers[1].btn = 0;
    pointers[1].x = 0;
    pointers[1].y = _fb_height;
    pointers[1].z = 0;
//    vid_movecursor(1,pointers[1].x,pointers[1].y);
}

public void pointer(uint64_t btn, uint64_t dx, uint64_t dy, uint64_t dz, uint64_t hand)
{
    pointers[hand].btn = btn & 0x7;
    pointers[hand].x += dx;
    pointers[hand].y += dy;
    pointers[hand].z += dz;
#if DEBUG
    dbg_printf("---------pointer %4x  ", btn);
    dbg_printf("%x %d,%d,%d\n", pointers[hand].btn,pointers[hand].x,pointers[hand].y,pointers[hand].z);
#endif
//    vid_movecursor(hand,pointers[hand].x,pointers[hand].y);
}
