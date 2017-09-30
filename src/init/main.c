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

public uint8_t _identity = false;
public uint8_t _rescueshell = false;

extern void services_init();

void task_init(int argc, char **argv)
{
    /* sys_ready() will fake a mountfs call to FS, wait for it's ack */
    mq_recv();


fid_t f=fopen("/etc/kbd/en_us", O_RDWR | O_EXCL);
dbg_printf("fopen ret %d errno %d\n", f, errno());
fseek(f,10,SEEK_SET);
fseek(f,8,SEEK_CUR);
fseek(f,-10,SEEK_CUR);
fseek(f,-10,SEEK_END);
rewind(f);
dbg_printf("ftell ret %d errno %d\n", ftell(f), errno());
f=fclose(f);
dbg_printf("fclose ret %d errno %d\n", f, errno());
dbg_printf("ftell ret %d errno %d\n", ftell(0), errno());

    if(_rescueshell) {
        /* create a TTY window for rescue shell */
//        ui_createwindow();
        /* replace ourself with shell */
//        exec("/bin/sh");
        // never return here.
    } else {
        if(_identity) {
            /* start first time turn on's set up task, wait until it returns */
//            system("/sys/bin/identity");
        }
        // load user services
        services_init();
    }
exit(0);
}
