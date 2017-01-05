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
#ifndef _ENV_C_
extern uint64_t nrphymax;   // number of hysical fragment pages
extern uint64_t nrmqmax;    // number of message queue pages
extern uint64_t nrirqmax;   // number of interrupt routing pages
extern uint64_t nrsrvmax;   // number of services pages
extern uint64_t nrlogmax;   // number of syslog_buf pages
extern uint8_t networking;  // should start net service on boot?
extern uint8_t sound;       // should start sound service?
extern uint8_t rescueshell; // boot rescue shell instead of init?
extern uint8_t identity;    // run first time setup
extern uint64_t debug;      // see below
extern uint64_t quantum;    // max CPU allocation time: 1/quantum sec
extern uint64_t fps;        // max frame per sec
extern uint64_t display;    // see below
#endif
#endif

/* debug levels */
#define DBG_NONE 0          // none, false
#define DBG_MEMMAP 1        // mm
#define DBG_THREADS 2       // th
#define DBG_ELF 4           // el
#define DBG_RTIMPORT 8      // ri
#define DBG_RTEXPORT 16     // re
#define DBG_IRQ 32          // ir
#define DBG_DEVICES 64      // de
#define DBG_SCHED 128       // sc
#define DBG_MSG 256         // ms
#define DBG_LOG 512         // lo

/* display options */
#define DSP_MONO_COLOR 1    // flat 2D color
#define DSP_STEREO_MONO 2   // grayscale red-cyan 3D anaglyph
#define DSP_STEREO_COLOR 3  // anything (polarized glass, VR helmet etc.)
