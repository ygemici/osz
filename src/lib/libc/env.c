/*
 * lib/libc/env.c
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
 * @brief Environment configuration parser routines
 */

#include <osZ.h>

// import environment configuration (mapped and linked by run-time linker)
public char *_environment=NULL;

/**
 * parse environment for a number value
 */
public uint64_t env_num(char *key, uint64_t def, uint64_t min, uint64_t max)
{
    if(_environment==NULL || key==NULL || key[0]==0)
        return def;
    char *env = _environment;
    char *env_end = _environment+__PAGESIZE;
    int i=strlen(key),s=false;
    uint64_t ret=def;
    // parse ascii text
    while(env < env_end && *env!=0) {
        // skip comments
        if((env[0]=='/'&&env[1]=='/') || env[0]=='#') { while(env<env_end && env[0]!=0 && env[0]!='\n') env++; }
        if(env[0]=='/'&&env[1]=='*') { env+=2; while(env<env_end && env[0]!=0 && env[-1]!='*' && env[0]!='/') env++; }
        // match key
        if(env+i<env_end && !memcmp(env,key,i) && env[i]=='=') {
            env+=i+1;
            if(*env=='-') { env++; s=true; }
            stdlib_dec((uchar *)env, &ret, min, max);
            return s ? -ret : ret;
        }
        env++;
    }
    return def;
}

/**
 * parse environment for a boolean value
 */
public bool_t env_bool(char *key, bool_t def)
{
    if(_environment==NULL || key==NULL || key[0]==0)
        return def;
    char *env = _environment;
    char *env_end = _environment+__PAGESIZE;
    int i=strlen(key);
    bool_t ret=def;

    // parse ascii text
    while(env < env_end && *env!=0) {
        // skip comments
        if((env[0]=='/'&&env[1]=='/') || env[0]=='#') { while(env<env_end && env[0]!=0 && env[0]!='\n') env++; }
        if(env[0]=='/'&&env[1]=='*') { env+=2; while(env<env_end && env[0]!=0 && env[-1]!='*' && env[0]!='/') env++; }
        // match key
        if(env+i<env_end && !memcmp(env,key,i) && env[i]=='=') {
            env+=i+1;
            // 1, true, enabled, on
            if((*env=='1'||*env=='t'||*env=='T'||*env=='e'||*env=='E'||
                (*env=='o'&&*(env+1)=='n')||(*env=='O'&&*(env+1)=='N')))
                ret=true;
            // 0, false, disabled, off
            if((*env=='0'||*env=='f'||*env=='F'||*env=='d'||*env=='D'||
                (*env=='o'&&*(env+1)=='f')||(*env=='O'&&*(env+1)=='F')))
                ret=false;
            break;
        }
        env++;
    }
    return ret;
}

/**
 * parse environment for a string value. The returned value malloc'd, must be freed by caller.
 */
public char *env_str(char *key, char *def)
{
    char *env = _environment;
    char *env_end = _environment+__PAGESIZE;
    int i=strlen(key),j=strlen(def)+1;
    char *ret=def,*s;
    if(_environment==NULL || key==NULL || key[0]==0)
        goto doalloc;
    // parse ascii text
    while(env < env_end && *env!=0) {
        // skip comments
        if((env[0]=='/'&&env[1]=='/') || env[0]=='#') { while(env<env_end && env[0]!=0 && env[0]!='\n') env++; }
        if(env[0]=='/'&&env[1]=='*') { env+=2; while(env<env_end && env[0]!=0 && env[-1]!='*' && env[0]!='/') env++; }
        // match key
        if(env+i<env_end && !memcmp(env,key,i) && env[i]=='=') {
            env+=i+1;
            for(s=env;s<env_end && *s!='\n' && *s!=0;s++);
            ret=env; j=s-env;
            break;
        }
        env++;
    }
doalloc:
    s=(char*)malloc(j+1);
    if(s!=NULL)
        memcpy(s,ret,j);
    return s;
}
