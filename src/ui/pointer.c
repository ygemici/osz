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
#include "keys.h"

extern uint64_t _fb_width;
extern uint64_t _fb_height;

// current modifier status
extern uint8_t keyflags;

// pointer status
typedef struct {
    uint16_t shape;
    uint16_t btn;
    int32_t x;
    int32_t y;
    int32_t z;
} pointer_t;

pointer_t pointers[2];

void reset_pointers()
{
    pointers[0].btn = 0;
    pointers[0].x = _fb_width/2;
    pointers[0].y = _fb_height/2;
    pointers[0].z = 0;
    vid_movecursor(0,pointers[0].x,pointers[0].y);
    pointers[1].btn = 0;
    pointers[1].x = 0;
    pointers[1].y = _fb_height;
    pointers[1].z = 0;
    vid_movecursor(1,pointers[1].x,pointers[1].y);
}

public void setcursor(uint8_t hand, uint8_t shape)
{
    if(hand>1) hand=1;
    if(shape<33) {
        pointers[hand].shape=shape;
        vid_setcursor(hand,shape);
    }
}

public void pointer(uint64_t btn, int32_t dx, int32_t dy, int32_t dz, uint8_t hand)
{
    if(hand>1) hand=1;
    if(hand==0 && keyflags&KEYFLAG_CTRL) hand=1;
    pointers[hand].btn = ((btn & 1)?1+hand:0) | ((btn & 2)?2-hand:0);
    pointers[hand].x += dx;
    if(pointers[hand].x<0) pointers[hand].x=0;
    if(pointers[hand].x>(int32_t)_fb_width) pointers[hand].x=(int32_t)_fb_width;
    pointers[hand].y -= dy;
    if(pointers[hand].y<0) pointers[hand].y=0;
    if(pointers[hand].y>(int32_t)_fb_height) pointers[hand].y=(int32_t)_fb_height;
    pointers[hand].z += dz;
#if DEBUG
    dbg_printf("---------pointer %4x  %d ", btn, hand);
    dbg_printf("%x %d,%d,%d\n", pointers[hand].btn,pointers[hand].x,pointers[hand].y,pointers[hand].z);
#endif
    vid_movecursor(hand,pointers[hand].x,pointers[hand].y);
    if(pointers[hand].shape==VC_default && pointers[hand].btn&1) setcursor(hand,VC_clicked);
    if(pointers[hand].shape==VC_clicked && !pointers[hand].btn&1) setcursor(hand,VC_default);
    if(pointers[hand].shape==VC_grab && pointers[hand].btn&1) setcursor(hand,VC_grabbing);
    if(pointers[hand].shape==VC_grabbing && !pointers[hand].btn&1) setcursor(hand,VC_grab);
}
