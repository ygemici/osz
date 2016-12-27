/*
 * core/env.h
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
 * @brief Core environment variables (defined in FS0:\BOOTBOOT\CONFIG)
 */

extern uint64_t nrphymax;
extern uint64_t nrmqmax;
extern uint64_t nrirqmax;
extern uint8_t networking;
extern uint8_t sound;
extern uint8_t rescueshell;
extern uint8_t identity;
extern uint64_t debug;
extern uint64_t quantum;
extern uint64_t fps;
extern uint64_t display;

/* debug levels */
#define DBG_NONE 0
#define DBG_MEMMAP 1
#define DBG_THREADS 2
#define DBG_ELF 4
#define DBG_RTIMPORT 8
#define DBG_RTEXPORT 16
#define DBG_IRQ 32
#define DBG_DEVICES 64
#define DBG_SCHED 128
#define DBG_MSG 256

/* display options */
#define DSP_MONO_MONO 0
#define DSP_MONO_COLOR 1
#define DSP_STEREO_MONO 2
#define DSP_STEREO_COLOR 3
