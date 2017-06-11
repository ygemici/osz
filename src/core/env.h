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

#ifndef _AS
#ifndef _ENV_C
extern uint64_t nrphymax;   // number of hysical fragment pages
extern uint64_t nrmqmax;    // number of message queue pages
extern uint64_t nrsrvmax;   // number of services pages
extern uint64_t nrlogmax;   // number of syslog_buf pages
extern uint64_t nropenmax;  // number of open file descriptors per thread
extern uint8_t syslog;      // should start syslog service on boot?
extern uint8_t networking;  // should start net service on boot?
extern uint8_t sound;       // should start sound service?
extern uint8_t rescueshell; // boot rescue shell instead of init?
extern uint8_t identity;    // run first time setup utility
extern uint64_t clocksource;// clock source
extern uint64_t debug;      // debug flags
extern uint64_t quantum;    // max CPU allocation time: 1/quantum sec
extern uint64_t fps;        // max frame per sec
extern uint64_t display;    // display type
extern uint64_t keymap;     // keymap
#endif
#endif

/* debug levels */
#define DBG_NONE     0      // none, false
#define DBG_MEMMAP   (1<<0) // mm 1
#define DBG_THREADS  (1<<1) // th 2
#define DBG_ELF      (1<<2) // el 4
#define DBG_RTIMPORT (1<<3) // ri 8
#define DBG_RTEXPORT (1<<4) // re 16
#define DBG_IRQ      (1<<5) // ir 32
#define DBG_DEVICES  (1<<6) // de 64
#define DBG_SCHED    (1<<7) // sc 128
#define DBG_MSG      (1<<8) // ms 256
#define DBG_LOG      (1<<9) // lo 512

/* display options */
#define DSP_MONO_COLOR   0  // mc flat 2D color
#define DSP_STEREO_MONO  1  // sm grayscale red-cyan 3D (anaglyph)
#define DSP_STEREO_COLOR 2  // sc real 3D (polarized glass, VR helmet etc. driver specific)
