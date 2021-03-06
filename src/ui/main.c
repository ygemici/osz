/*
 * ui/main.c
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
 * @brief User Interface initialization and message dispatch
 */
#include <osZ.h>
#include <sys/driver.h>

extern char _binary_logo_start;
extern void keymap_init();
extern void reset_pointers();

// filled by run-time linker
public void* _screen_ptr;
public uint64_t _fb_width;
public uint64_t _fb_height;
public uint8_t _display_type;
uint8_t lefthanded=false;
char *keymap_type;

public void devprogress(fid_t fd, blkcnt_t out, blkcnt_t total){}
public void opentty(){}
public void openwin(){}
public void openwrd(){}

void task_init(int argc, char **argv)
{
    //environment
    lefthanded = env_bool("lefthanded",false);
//    keymap_type = env_str("keymap","en_us");

    mq_recv();
    
    keymap_init();
    reset_pointers();
}
