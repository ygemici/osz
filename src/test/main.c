/*
 * test/main.c
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
 * @brief Application to test services and libraries
 */
#include <osZ.h>
#include <sys/driver.h>

void mmap_test();
void string_test();
void stdlib_test();
void stdio_test();
void unistd_test();

/* normally we should use getcwd() */
typedef struct {
    fid_t cwdir;
    fid_t of;
    fid_t lf;
    fid_t *fd;
} _fd_t;
extern _fd_t _fd;

int main(int argc, char**argv)
{
    /* do we have a current working directory? If no, then we were started as init service */
    if(!_fd.cwdir) {
        /* wait until FS sends us an SYS_ack message after the SYS_mountfs call */
        mq_recv();
    }

    dbg_printf("\n------------------------- TESTS ----------------------------\n");
    //do tests
//    mmap_test();
    string_test();
    dbg_printf(  "----------------------- TESTS END --------------------------\n\n");

    /* if we are running as init, exiting task will trigger poweroff, exactly what we want. */
    return 0;
}
