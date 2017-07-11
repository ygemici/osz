/*
 * fs/vfs.c
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
 * @brief Virtual File System implementation
 */
#include <osZ.h>
#include "vfs.h"
#include "devfs.h"
#include "cache.h"

/* path manipulations buffer */
extern uint64_t _pathmax;
char *tmppath=NULL;

/* filesystem parsers */
uint16_t nfsdrvs = 0;
fsdrv_t *fsdrvs = NULL;

/* mount points */
uint64_t nmtab = 0;
mount_t *mtab = NULL;

/* files and directories */
uint64_t nfcbs = 0;
fcb_t *fcbs = NULL;

/* open files, returned by lsof */
uint64_t nfiles = 0;
openfile_t *files = NULL;

/**
 * locate a file
 */
public fid_t vfslocate(fid_t parent, char *name, uint64_t type)
{
    uint64_t i,j,k=0,l=0;
    ino_t in;

    seterr(SUCCESS);
    if(tmppath==NULL) {
        tmppath=(char*)malloc(_pathmax<512?512:_pathmax);
        if(!tmppath || errno)
            return -1;
    }
    if(parent>=nfcbs) {
        seterr(ENXIO);
        return -1;
    }
    if(name==NULL || name[0]==0)
        return parent;
    /* TODO: canonize path */
    strcpy(tmppath, fcbs[parent].path);
    strcat(tmppath, name+(name[0]=='/'?1:0));
    //canonize(tmppath);
dbg_printf("tmppath '%s'\n",tmppath);

    /* find the longest match in mtab */
    for(i=0;i<nmtab;i++) {
        j=strlen(mtab[i].fs_file);
        if(j>k && !memcmp(mtab[i].fs_file, tmppath, j)) {
            l=j;
            k=i;
        }
    }
dbg_printf("%d / '%s'\n",k,tmppath+l);
dbg_printf("s %d %x\n",mtab[k].fs_type,fsdrvs[mtab[k].fs_type].locate);
    if(l==0 || mtab[k].fs_type>nfsdrvs || fsdrvs[mtab[k].fs_type].locate==NULL) {
        seterr(ENODEV);
        return -1;
    }
    
    /* pass the remaining path to filesystem driver */
    in=(*fsdrvs[mtab[k].fs_type].locate)(&mtab[k], tmppath+l, type);
    if(in==-1) {
        seterr(ENOENT);
        return -1;
    }
dbg_printf("fslocate %d\n",in);
return 0;
}

/**
 * Register a filesystem parser
 */
public uint16_t _fs_reg(const fsdrv_t *fs)
{
    fsdrvs=(fsdrv_t*)realloc(fsdrvs,++nfsdrvs*sizeof(fsdrv_t));
    if(!fsdrvs || errno)
        abort();
    memcpy((void*)&fsdrvs[nfsdrvs-1], (void*)fs, sizeof(fsdrv_t));
    return nfsdrvs-1;
}

/**
 * return filesystem parser
 */
uint16_t _fs_get(char *name)
{
    int i;
    for(i=0;i<nfsdrvs;i++)
        if(!strcmp(fsdrvs[i].name, name))
            return i;
    return 0;
}

/**
 * add a file to fcb list
 */
fid_t fcb_add(const fcb_t *fcb)
{
    uint64_t i;
    for(i=0;i<nfcbs;i++) {
        if( fcbs[i].path!=NULL && !strcmp(fcbs[i].path,fcb->path))
            return i;
    }
    fcbs=(fcb_t*)realloc(fcbs,++nfcbs*sizeof(fcb_t));
    if(!fcbs || errno)
        abort();
    memcpy((void*)&fcbs[nfcbs-1], (void*)fcb, sizeof(fcb_t));
    return nfcbs-1;
}

uint64_t mtab_add(char *dev, char *file, char *opts)
{
    mount_t *mnt;
    fcb_t fcb;
    void *block;
    ino_t devidx;
    uint64_t i,n=nmtab;
    if(strncmp(dev,"/dev/",5))
        return -1;
    devidx=devfs_locate(NULL, dev+5, S_IFBLK);
    if(devidx>=ndevdir)
        return -1;
    // get first sector of devdir[devidx]
    block=cache_getblock(devidx, 0);
    if(block==NULL)
        return -1;
    // detect filesystem type
    i=0; while(i<nfsdrvs && (fsdrvs[i].detect==NULL || !(*fsdrvs[i].detect)(block))) i++;
    if(i>=nfsdrvs)
        return -1;
    dbg_printf("parsed dev='%s' path='%s' fs='%s' opts='%s'\n",dev,file,fsdrvs[i].name,opts);
    mtab=(mount_t*)realloc(mtab,++nmtab*sizeof(mount_t));
    if(!mtab || errno)
        abort();
    mnt=&mtab[n];
    mnt->fs_file=strdup(file);
    if(mnt->fs_file[strlen(mnt->fs_file)-1]!='/') {
        mnt->fs_file=(char*)realloc(mnt->fs_file,strlen(mnt->fs_file)+1);
        mnt->fs_file[strlen(mnt->fs_file)]='/';
    }
    mnt->fs_spec=devidx;
    mnt->fs_type=i;

    // when mounting root, also mount /dev. It's path is fixed, mount point cannot be changed
    if(file[0]=='/' && file[1]==0) {
        // also add to FCB list. Uninitialized rootdir and chdir will point here
        fcb.path=mnt->fs_file;
        fcb.nlink=1;
        fcb.type=VFS_FCB_TYPE_DIRECTORY;
        fcb.directory.mount=n;
        // failsafe. Root must be the first
        if(fcb_add(&fcb)!=0) {
#if DEBUG
            dbg_printf("Root directory must be the first FCB!\n");
#endif
            abort();
        }
        mtab=(mount_t*)realloc(mtab,++nmtab*sizeof(mount_t));
        if(!mtab || errno)
            abort();
        mnt=&mtab[n+1];
        mnt->fs_file="/dev/";
        mnt->fs_spec=-1;
        mnt->fs_type=_fs_get("devfs");
    }
    return n;
}

/**
 * parse /etc/fstab and build file system mount points
 */
void vfs_fstab(char *ptr, size_t size)
{
    char *dev, *file, *opts, *end, pass;
    char *fstab = (char*)malloc(size);
    // create a copy 'cos we are going to convert strings to asciiz in it
    if(!fstab || errno)
        abort();
    memcpy(fstab,ptr,size);
    end=fstab+size;
    // make sure we call mtab_add in order
    for(pass='1';pass<='9';pass++) {
        ptr=fstab;
        while(ptr<end) {
            while(*ptr=='\n'||*ptr=='\t'||*ptr==' ') ptr++;
            // skip comments
            if(*ptr=='#') {
                for(ptr++;ptr<end && *(ptr-1)!='\n';ptr++);
                continue;
            }
            // block device
            dev=ptr;
            while(ptr<end && *ptr!=0 && *ptr!='\t' && *ptr!=' ') ptr++;
            if(*ptr=='\n') continue;
            *ptr=0; ptr++;
            while(ptr<end && (*ptr=='\t' || *ptr==' ')) ptr++;
            if(*ptr=='\n') continue;
            // mount point
            file=ptr;
            while(ptr<end && *ptr!=0 && *ptr!='\t' && *ptr!=' ') ptr++;
            if(*ptr=='\n') continue;
            *ptr=0; ptr++;
            while(ptr<end && (*ptr=='\t' || *ptr==' ')) ptr++;
            if(*ptr=='\n') continue;
            // opts and access
            opts=ptr;
            while(ptr<end && *ptr!=0 && *ptr!='\t' && *ptr!=' ') ptr++;
            if(*ptr=='\n') continue;
            *ptr=0; ptr++;
            while(ptr<end && (*ptr=='\t' || *ptr==' ')) ptr++;
            if(*ptr=='\n') continue;
            // pass
            if(*ptr==pass)
                mtab_add(dev,file,opts);
            ptr++;
        }
    }
    free(fstab);
}


#if DEBUG
void vfs_dump()
{
    int i;
    dbg_printf("Filesystems %d:\n",nfsdrvs);
    for(i=0;i<nfsdrvs;i++)
        dbg_printf("%3d. '%s' %s %x\n",i,fsdrvs[i].name,fsdrvs[i].desc,fsdrvs[i].detect);

    dbg_printf("\nMounts %d:\n",nmtab);
    for(i=0;i<nmtab;i++)
        dbg_printf("%3d. '%s' %s %s\n",i,devdir[mtab[i].fs_spec].name,fsdrvs[mtab[i].fs_type].name,mtab[i].fs_file);

    dbg_printf("\nFile Control Blocks %d:\n",nfcbs);
    for(i=0;i<nfcbs;i++) {
        dbg_printf("%3d. %x ",i,fcbs[i].type);
        switch(fcbs[i].type) {
            case VFS_FCB_TYPE_FILE:
                dbg_printf("file %s\n",fcbs[i].path);
                break;
            case VFS_FCB_TYPE_PIPE:
                dbg_printf("pipe %s\n",fcbs[i].path);
                break;
            case VFS_FCB_TYPE_DIRECTORY:
                dbg_printf("directory %s\n",fcbs[i].path);
                break;
            default:
                dbg_printf("unknown type? %s\n",fcbs[i].path);
        }
    }

    dbg_printf("\nOpen files %d:\n",nfiles);
    for(i=0;i<nfiles;i++)
        dbg_printf("%3d. pid %x fid %5d seek %d\n",i,files[i].pid,files[i].fid,files[i].pos);

    dbg_printf("\n");

dbg_printf("locate %d\n",vfslocate(0,"/etc/kbd/",0));
}
#endif
