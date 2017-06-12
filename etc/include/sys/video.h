/*
 * sys/video.h
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
 * @brief Video driver messages
 */

#ifndef _SYS_VIDEO_H
#define _SYS_VIDEO_H 1

/*** low level codes in rdi for syscall instruction ***/
#define VID_flush 1
#define VID_showcursor 2
#define VID_loadcursor 3
#define VID_setcursor 4

/* video cursor shapes */
#define VC_none -1
#define VC_default 0
#define VC_pointer 1
#define VC_clicked 2
#define VC_help 3
#define VC_kill 4
#define VC_notallowed 5
#define VC_cell 6
#define VC_crosshair 7
#define VC_text 8
#define VC_vtext 9
#define VC_alias 10
#define VC_copy 11
#define VC_move 12
#define VC_scroll 13
#define VC_colresize 14
#define VC_rowresize 15
#define VC_nresize 16
#define VC_eresize 17
#define VC_sresize 18
#define VC_wresize 19
#define VC_neresize 20
#define VC_nwresize 21
#define VC_seresize 22
#define VC_swresize 23
#define VC_ewresize 24
#define VC_nsresize 25
#define VC_neswresize 26
#define VC_nwseresize 27
#define VC_zoomin 28
#define VC_zoomout 29
#define VC_grab 30
#define VC_grabbing 31
#define VC_progress 32 //remaining shapes animated

/*** libc implementation prototypes */
#ifndef _AS
void vid_swapbuf();
void vid_showcursor(int hand,int x,int y);
void vid_loadcursor(char *file);
void vid_setcursor(int shape);
#endif

#endif /* sys/video.h */
