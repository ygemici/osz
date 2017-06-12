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

/*** libc implementation prototypes */
#ifndef _AS
void vid_swapbuf();
void vid_showcursor(int x,int y);
void vid_loadcursor(char *file);
void vid_setcursor(int shape);
#endif

#endif /* sys/video.h */
