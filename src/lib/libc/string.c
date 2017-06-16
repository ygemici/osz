/*
 * lib/libc/string.c
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
 * @brief Function implementations for string.h. Note there's a (platform)/string.S
 */
#include <osZ.h>

public char *errnums[] = { "SUCCESS", "EPERM", "ENOENT", "ESRCH", "EINTR", "EIO", "ENXIO", "E2BIG", "ENOEXEC",
    "EBADF", "ECHILD", "EAGAIN", "ENOMEM", "EACCES", "EFAULT", "ENOTBLK", "EBUSY", "EEXIST", "EXDEV",
    "ENODEV", "ENOTDIR", "EISDIR", "EINVAL", "ENFILE", "EMFILE", "ENOTTY", "ETXTBSY", "EFBIG", "ENOSPC",
    "ESPIPE", "EROFS", "EMLNK", "EPIPE", "EDOM", "ERANGE" };

public char *sigs[] = { "NONE", "HUP", "INT", "QUIT", "ILL", "TRAP", "ABRT", "EMT", "FPE", "KILL", "BUS", "SEGV",
    "SYS", "PIPE", "ALRM", "TERM", "URG", "STOP", "TSTP", "CONT", "CHLD", "TTIN", "TTOU", "IO", "XCPU", "XFSZ",
    "VTALRM", "PROF", "WINCH", "INFO", "USR1", "USR2" };

char *strerror(int errnum)
{
    return errnum>=0 && errnum<sizeof(errnums) ? errnums[errnum] : "EUNKWN";
}

char *strsignal(int sig)
{
    return sig>=0 && sig<sizeof(sigs) ? sigs[sig] : "?";
}

char *strdup(char *s)
{
    int i=strlen(s)+1;
    char *s2=(char *)malloc(i);
    if(s2!=NULL)
        memcpy(s2,s,i);
    return s2;
}

char *strndup(char *s, size_t n)
{
    int i=strnlen(s,n);
    char *s2=(char *)malloc(i+1);
    if(s2!=NULL) {
        memcpy(s2,s,i);
        s2[i]=0;
    }
    return s2;
}

char *basename(char *s)
{
    return s;
}

char *dirname(char *s)
{
    return s;
}

void *memmem (void *haystack, size_t hl, void *needle, size_t nl)
{
    char *c=haystack;
    if(haystack==NULL || needle==NULL || hl==0 || nl==0 || nl>hl)
        return NULL;
    hl-=nl;
    while(hl) {
        if(!memcmp(c,needle,nl))
            return c;
        c++; hl--;
    }
    return NULL;
}

char *strstr (char *haystack, char *needle)
{
    return memmem(haystack, strlen(haystack), needle, strlen(needle));
}

