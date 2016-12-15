#ifndef _ACCESS_H
#define _ACCESS_H 1

#define SRV_FS 0x200

#define SYS_read (SRV_FS | 0)
#define SYS_write (SRV_FS | 1)
#define SYS_open (SRV_FS | 2)
#define SYS_close (SRV_FS | 3)
#define SYS_stat (SRV_FS | 4)
#define SYS_fstat (SRV_FS | 5)
#define SYS_lstat (SRV_FS | 6)
#define SYS_poll (SRV_FS | 7)
#define SYS_lseek (SRV_FS | 8)
#define SYS_mmap (SRV_FS | 9)
#define SYS_mprotect (SRV_FS | 10)
#define SYS_munmap (SRV_FS | 11)
#define SYS_ioctl (SRV_FS | 12)
#define SYS_pipe (SRV_FS | 13)
#define SYS_select (SRV_FS | 14)
#define SYS_dup (SRV_FS | 15)
#define SYS_dup2 (SRV_FS | 16)
#define SYS_sendfile (SRV_FS | 17)

#endif
