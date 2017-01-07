/*
 * sys/sysinfo.h
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
 * @brief OS/Z System Information Structure
 */

#ifndef	_SYS_SYSINFO_H
#define	_SYS_SYSINFO_H	1

#include <lastbuild.h>

#ifndef _AS
typedef struct {
    uint8_t display;        // display type
    uint8_t fps;            // maximum frame per second
    uint8_t rescueshell;    // rescue shell requested flag
    uint8_t reserved;
    uint32_t debug;         // debug flags
    uint64_t screen_ptr;    // screen virtual address ("SYS" and "UI")
    uint64_t fb_ptr;        // framebuffer virtual address ("SYS" only)
    uint64_t fb_width;      // framebuffer width
    uint64_t fb_height;     // framebuffer height
    uint64_t fb_scanline;   // framebuffer line size
    uint64_t quantum;       // max time a task can allocate CPU 1/quantum
    uint64_t quantum_cnt;   // total number of task switches
    uint64_t freq;          // timer freqency, task switch at freq/quantum
    uint64_t ticks[2];      // overall jiffies counter
    uint64_t timestamp_s;   // UTC timestamp
    uint64_t timestamp_ns;  // UTC timestamp nanosec fraction
    uint64_t srand[4];      // random seed bits
    uint64_t systables[16]; // platform specific addresses
} __attribute__((packed)) sysinfo_t;

sysinfo_t *sysinfo();                   // query system information
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
#define DSP_MONO_COLOR   1  // flat 2D color
#define DSP_STEREO_MONO  2  // grayscale red-cyan 3D anaglyph
#define DSP_STEREO_COLOR 3  // anything 3D (polarized glass, VR helmet etc.)

/*** Platform independent ***/
#define sysinfo_display 0
#define sysinfo_fps 1
#define sysinfo_rescueshell 2
#define sysinfo_debug 4
#define sysinfo_screen 8
#define sysinfo_fb_ptr 16
#define sysinfo_fb_w 24
#define sysinfo_fb_h 32
#define sysinfo_fb_s 40
#define sysinfo_quantum 48
#define sysinfo_quantum_cnt 56
#define sysinfo_freq 64
#define sysinfo_jiffy_lo 72
#define sysinfo_jiffy_hi 80
#define sysinfo_ts_s 88
#define sysinfo_ts_ns 96
#define sysinfo_srand0 104
#define sysinfo_srand1 112
#define sysinfo_srand2 120
#define sysinfo_srand3 128

/*** Platform specific ***/
#define sysinfo_systables 136

#ifdef OSZ_ARCH_Aarch64
#endif

#ifdef OSZ_ARCH_x86_64
#define sysinfo_acpi_ptr 136
#define sysinfo_smbi_ptr 144
#define sysinfo_efi_ptr 152
#define sysinfo_mp_ptr 160
#define sysinfo_apic_ptr 168
#define sysinfo_hpet_ptr 176

#define systable_acpi_ptr 0
#define systable_smbi_ptr 1
#define systable_efi_ptr 2
#define systable_mp_ptr 3
#define systable_apic_ptr 4
#define systable_hpet_ptr 5
#endif

#endif /* sys/sysinfo.h */
