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
 * @brief Function implementations for string.h. Note there's a (platform)/string.S too
 */
#include <osZ.h>
#include "strings.h"

/* lowercase conversion table for UNICODE, see string.S tolower */
public uint32_t translatelower[] = {
    0x178/*Ÿ*/,0xFF/*ÿ*/, 0 };

/* Return a string describing the meaning of the `errno' code in ERRNUM.  */
char *strerror(int errnum)
{
    return errnum>=0 && errnum<sizeof(errstrs)/sizeof(errstrs[0]) ? errstrs[errnum] : "Unknown error";
}

/* Return a string describing the meaning of the signal number in SIG.  */
char *strsignal(int sig)
{
    return sig>=0 && sig<sizeof(sigs)/sizeof(sigs[0]) ? sigs[sig] : "?";
}

/* Duplicate S, returning an identical malloc'd string.  */
char *strdup(const char *s)
{
    int i=strlen(s)+1;
    char *s2=(char *)malloc(i);
    if(s2!=NULL)
        memcpy(s2,(void*)s,i);
    return s2;
}

/* Duplicate S, returning an identical malloc'd string.  */
char *strndup(const char *s, size_t n)
{
    int i=strnlen(s,n);
    char *s2=(char *)malloc(i+1);
    if(s2!=NULL) {
        memcpy(s2,(void*)s,i);
        s2[i]=0;
    }
    return s2;
}

/* Find the first occurrence of NEEDLE in HAYSTACK.
   NEEDLE is NEEDLELEN bytes long;
   HAYSTACK is HAYSTACKLEN bytes long.  */
void *memmem (const void *haystack, size_t hl, const void *needle, size_t nl)
{
    char *c=(char*)haystack;
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

/* Find the first occurrence of NEEDLE in HAYSTACK.  */
char *strstr (const char *haystack, const char *needle)
{
    return memmem(haystack, strlen(haystack), needle, strlen(needle));
}

/* Similar to `strstr' but this function ignores the case of both strings.  */
char *strcasestr (const char *haystack, const char *needle)
{
    char *c=(char*)haystack;
    size_t hl=strlen(haystack);
    size_t nl=strlen(needle);
    if(haystack==NULL || needle==NULL || hl==0 || nl==0 || nl>hl)
        return NULL;
    hl-=nl;
    while(hl) {
        if(!strncasecmp(c,needle,nl))
            return c;
        c++; hl--;
    }
    return NULL;
}

/* Return filename part of path */
char *basename(const char *s)
{
    if(s==NULL) return NULL;
    char *r;
    int i,e=strlen(s)-1;
    if(s[e]=='/') e--;
    for(i=e;i>1 && s[i-1]!='/';i--);
    if(i==e) return NULL;
    r=(char*)malloc(e-i+1);
    memcpy(r,(void*)s+i,e-i);
    return r;
}

/* Return directory part of path */
char *dirname(const char *s)
{
    if(s==NULL) return NULL;
    char *r;
    int i,e=strlen(s)-1;
    if(s[e]=='/') e--;
    for(i=e;i>0 && s[i]!='/';i--);
    if(i==e||i==0) return NULL;
    r=(char*)malloc(i);
    memcpy(r,(void*)s,i-1);
    return r;
}

/* helper for tokenizer functions */
private char *_strtok_r (char *s, char *d, char **p, uint8_t skip)
{
    int c, sc;
    char *tok, *sp;

    if(d==NULL || (s==NULL&&(s=*p)==NULL)) return NULL;
again:
    c=*s++;
    for(sp=(char *)d;(sc=*sp++)!=0;) {
        if(c==sc) {
            if(skip) goto again;
            else { *p=s; *(s-1)=0; return s-1; }
        }
    }

    if (c==0) { *p=NULL; return NULL; }
    tok=s-1;
    while(1) {
        c=*s++;
        sp=(char *)d;
        do {
            if((sc=*sp++)==c) {
                if(c==0) s=NULL;
                else *(s-1)=0;
                *p=s;
                return tok;
            }
        } while(sc!=0);
    }
    return NULL;
}

/* Divide S into tokens separated by characters in DELIM.  */
char *strtok (char *s, char *delim)
{
    char *p=s;
    return _strtok_r (s, delim, &p, 1);
}

char *strtok_r (char *s, char *delim, char **ptr)
{
    return _strtok_r (s, delim, ptr, 1);
}

/* Return the next DELIM-delimited token from *STRINGP,
   terminating it with a '\0', and update *STRINGP to point past it.  */
char *strsep (char **stringp, char *delim)
{
    return _strtok_r (*stringp, delim, stringp, 0);
}
