/*
 * init/main.c
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
 * @brief Init System Service
 */
#include <osZ.h>

public uint8_t _rescueshell = false;

public void start(){}
public void stop(){}
public void restart(){}
public void status(){}

void task_init()
{
    //wait for sys_ready() to send an SYS_ack
    mq_recv();

//block for now
    mq_recv();
    if(_rescueshell) {
        breakpoint;
    } else {
ee:goto ee;
    }
}
