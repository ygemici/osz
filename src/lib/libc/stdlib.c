/*
 * lib/libc/stdlib.c
 *
 * Copyright 2017 CC-by-nc-sa bztsrc@github
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
 * @brief Function implementations for stdlib.h
 */
#include <osZ.h>

public char _osver[192];
public uint64_t _bogomips = 1000;
public uint64_t _alarmstep = 1000;
public uint64_t errn;
#if DEBUG
/* debug flags */
public uint32_t _debug;
#endif

/**
 * set error status, errno
 */
public void seterr(int e)
{
    errn = e;
}

/**
 * get error status, errno
 */
public int errno()
{
    return errn;
}

typedef void (*atexit_t)(void);
public int atexit_num = 0;
public atexit_t *atexit_hooks = NULL;

/* NOTE: that memory allocation functions are macros, see stdlib.h */

/**
 * Helper functions to number convert.
 */
unsigned char *stdlib_hex(unsigned char *s, uint64_t *v, uint64_t min, uint64_t max)
{
    if(*s=='0' && *(s+1)=='x')
        s+=2;
    *v=0;
    do{
        *v <<= 4;
        if(*s>='0' && *s<='9')
            *v += (uint64_t)((unsigned char)(*s)-'0');
        else if(*s >= 'a' && *s <= 'f')
            *v += (uint64_t)((unsigned char)(*s)-'a'+10);
        else if(*s >= 'A' && *s <= 'F')
            *v += (uint64_t)((unsigned char)(*s)-'A'+10);
        s++;
    } while((*s>='0'&&*s<='9')||(*s>='a'&&*s<='f')||(*s>='A'&&*s<='F'));
    if(*v < min)
        *v = min;
    if(max!=0 && *v > max)
        *v = max;
    return s;
}

unsigned char *stdlib_dec(unsigned char *s, uint64_t *v, uint64_t min, uint64_t max)
{
    if(*s=='0' && *(s+1)=='x')
        return stdlib_hex(s+2, v, min, max);
    *v=0;
    do{
        *v *= 10;
        *v += (uint64_t)((unsigned char)(*s)-'0');
        s++;
    } while(*s>='0'&&*s<='9');
    if(*v < min)
        *v = min;
    if(max!=0 && *v > max)
        *v = max;
    return s;
}

/* Convert a string to an integer.  */
int atoi(char *c)
{
    uint64_t r;
    int s=false;
    if(*c=='-') { s=true; c++; }
    stdlib_dec((uchar *)c, &r, 0, 0xFFFFFFFFULL);
    return s? -r : r;
}

/* Convert a string to a long integer.  */
long atol(char *c)
{
    uint64_t r;
    int s=false;
    if(*c=='-') { s=true; c++; }
    stdlib_dec((uchar *)c, &r, 0, 0xFFFFFFFFFFFFFFFFULL);
    return s? -r : r;
}

/* Convert a string to a long long integer.  */
long long atoll(char *c)
{
    return (long long)atol(c);
}

/* Do a binary search for KEY in BASE, which consists of NMEMB elements
   of SIZE bytes each, using CMP to perform the comparisons.  */
void *bsearch(void *key, void *base, size_t nmemb, size_t size, int (*cmp)(void *, void *))
{
    uint64_t s=0, e=nmemb;
    int ret;
        while (s<e) {
        uint64_t m=s+(e-s)/2;
        ret = cmp(key, base + m*s);
        if (ret<0) e=m; else
        if (ret>0) s=m+1; else
            return (void *)base + m*s;
    }
    return NULL;
}

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
    if(!atexit_hooks || errno())
        return errno();
    atexit_hooks[atexit_num++] = func;
    return SUCCESS;
}

/**
 *  Call all functions registered with `atexit',
 *  in the reverse of the order in which they were registered,
 *  perform stdio cleanup, and terminate program execution with STATUS.
 */
void _atexit()
{
    int i;
    /* call atexit handlers in reverse order */
    if(atexit_num>0)
        for(i=atexit_num;i>=0;--i)
            (*atexit_hooks[i])();
}

