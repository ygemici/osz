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

char buff[65536];

void dumpstat(stat_t *st)
{
    if(st==NULL)
        return;
    dbg_printf("    st_dev %d st_ino %d st_mode %4x\n",st->st_dev,st->st_ino,st->st_mode);
    dbg_printf("    st_type '%c%c%c%c' st_mime '%s'\n",st->st_type[0],st->st_type[1],st->st_type[2],st->st_type[3],
        st->st_mime);
    dbg_printf("    st_nlink %d st_size %d st_blksize %d st_blocks %d\n",st->st_nlink,st->st_size,st->st_blksize,
        st->st_blocks);
    dbg_printf("    st_atime %d st_ctime %d st_mtime %d\n",st->st_atime,st->st_ctime,st->st_mtime);
    dbg_printf("    st_owner %U\n", &st->st_owner);
}

void dumpdirent(dirent_t *de)
{
    if(de==NULL)
        return;
    dbg_printf("    d_dev %d\td_ino %d\td_type %02x",de->d_dev,de->d_ino,de->d_type);
    dbg_printf("\td_icon '%s'\n    d_len %d\td_name '%s'\n",de->d_icon,de->d_len,de->d_name);
}

void fs_test()
{
    fid_t f;
    f=fopen("/sys/etc/valami/megint",O_CREAT);
    dbg_printf("fopen ret %d errno %d %s\n", f, errno(), strerror(errno()));
/*
    dirent_t *de;
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

    stat_t *st=lstat("/etc");
    dbg_printf("lstat(/etc) ret %x errno %d %s\n", st, errno(), strerror(errno()));
    dumpstat(st);
    dbg_printf("fopen(/etc/kbd/en_us, O_RDWR)\n");
    f=fopen("/etc/kbd/en_us", O_RDWR);
    dbg_printf("fopen ret %d errno %d %s\n", f, errno(), strerror(errno()));
    fid_t f2=dup(f);
    dbg_printf("dup ret %d errno %d %s\n", f2, errno(), strerror(errno()));
    f2=dup2(f,f2);
    dbg_printf("dup2 ret %d errno %d\n", f2, errno());
    size_t s=fread(f,&buff,65536);
    dbg_printf("fread ret %d errno %d\n%s\n", s, errno(),buff);
    st=fstat(f);
    dbg_printf("fstat(%d) ret %x errno %d %s\n", f, st, errno(), strerror(errno()));
    dumpstat(st);
//    f=fclose(f);
    dbg_printf("fclose ret %d errno %d\n\n", f, errno());

//    f=fcloseall();
//    dbg_printf("fcloseall ret %d errno %d\n\n", f, errno());
    
    f=opendir("/dev");
    dbg_printf("opendir(/dev) ret %d errno %d\n\n", f, errno());
    de=(dirent_t*)1;
    while(de!=NULL) {
        de=readdir(f);
        dbg_printf("readdir(%d) ret %x errno %d %s\n", f, de, errno(), strerror(errno()));
        dumpdirent(de);
    }
    closedir(f);

    f=opendir("/etc");
    dbg_printf("opendir(/etc) ret %d errno %d\n\n", f, errno());
    de=(dirent_t*)1;
    while(de!=NULL) {
        de=readdir(f);
        dbg_printf("readdir(%d) ret %x errno %d %s\n", f, de, errno(), strerror(errno()));
        dumpdirent(de);
    }
//    closedir(f);

    mq_call(SRV_FS, SYS_fsdump);
    mq_send(SRV_FS, SYS_exit);
    mq_call(SRV_FS, SYS_fsdump);
*/
}
