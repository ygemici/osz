/*
 * test/fs.c
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
 * @brief File system tests
 */
#include <osZ.h>
#include <sys/driver.h>

void fs_test()
{
    dbg_printf("fopen(/etc/kbd/en_us, O_RDWR|OEXCL)\n");
    fid_t f=fopen("/etc/kbd/en_us", O_RDWR | O_EXCL);
    dbg_printf("fopen ret %d errno %d\n", f, errno());
    f=fclose(f);
    dbg_printf("fclose ret %d errno %d\n", f, errno());
    
    f=mount("/dev/valami", "/etc", NULL);
    dbg_printf("mount ret %d errno %d\n", f, errno());
    
    f=mount("/dev/zero", "/valami", NULL);
    dbg_printf("mount ret %d errno %d\n", f, errno());
    
    f=mount("/dev/zero", "/etc", NULL);
    dbg_printf("mount ret %d errno %d\n", f, errno());
    
    f=mount("/dev/root", "/", NULL);
    dbg_printf("mount ret %d errno %d\n", f, errno());
    
    f=mknod("stdin",31,O_RDWR,1, 0);
    dbg_printf("mknod ret %d errno %d\n", f, errno());
}
