/*
 * lib/libc/exit.c
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
 * @brief Exit routine
 */
#include <osZ.h>
#include <syscall.h>

extern void _exit(int code);

typedef void (*atexit_t)(void);

public int atexit_num = 0;
public atexit_t *atexit_hooks = NULL;

/**
 *  Register a function to be called when `exit' is called.
 */
int atexit (void (*func) (void))
{
    /* POSIX allows multiple registrations. OS/Z don't. */
    int i;
    for(i=0;i<atexit_num;i++)
        if(atexit_hooks[i] == func)
            return 1;
    atexit_hooks = realloc(atexit_hooks, (atexit_num+1)*sizeof(atexit_t));
    if(!atexit_hooks || errno)
        return errno;
    atexit_hooks[atexit_num++] = func;
    return SUCCESS;
}

/**
 *  Call all functions registered with `atexit' and `on_exit',
 *  in the reverse of the order in which they were registered,
 *  perform stdio cleanup, and terminate program execution with STATUS.
 */
void exit(int code)
{
    int i;
    /* call atexit handlers in reverse order */
    if(atexit_num>0)
        for(i=atexit_num;i>=0;--i)
            (*atexit_hooks[i])();
    _exit(code);
    /* make gcc happy */
    while(1);
}
