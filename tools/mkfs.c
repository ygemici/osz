/*
 * tools/mkfs.c
 *
 * Copyright 2017 CC-by-nc-sa-4.0 bztsrc@github
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
 * @brief Small utility to create FS/Z disk images
 *
 *  Compile: gcc mkfs.c -o mkfs
 *
 * Without arguments prints out usage.
 *
 * It's a minimal implementation, has several limitations compared to the FS/Z spec.
 * For example it supports only 24 entries per directory (only inlined entries in inode).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include "../etc/include/fsZ.h"
#include "../etc/include/crc32.h"

//-------------DATA-----------
char *diskname;     //entire disk
char *stage1;       //boot sector code
char *espfile;      //EFI System Partition imagefile
char *sysfile;      //system partition imagefile
char *usrfile;      //usr partition imagefile
char *varfile;      //var partition imagefile
char *homefile;     //home partition imagefile

unsigned int secsize=FSZ_SECSIZE;
FILE *f;
char *fs;
int size;
long int li=0,read_size=0;
long int ts;   // current timestamp in microsec
char *emptysec;// empty sector
int initrd;

//-------------CODE-----------

/* Misc functions */
/**
 * read a file into memory, returning buffer. Also sets read_size
 */
char* readfileall(char *file)
{
    char *data=NULL;
    FILE *f;
    read_size=0;
    f=fopen(file,"r");
    if(f){
        fseek(f,0L,SEEK_END);
        read_size=ftell(f);
        fseek(f,0L,SEEK_SET);
        data=(char*)malloc(read_size+secsize+1);
        if(data==NULL) { printf("mkfs: Unable to allocate %ld memory\n",read_size+secsize+1); exit(1); }
        memset(data,0,read_size+secsize+1);
        fread(data,read_size,1,f);
        fclose(f);
    }
    return data;
}
/**
 * get or set integer values in a char array
 */
int getint(char *ptr) { return (unsigned char)ptr[0]+(unsigned char)ptr[1]*256+(unsigned char)ptr[2]*256*256+ptr[3]*256*256*256; }
void setint(int val, char *ptr) { memcpy(ptr,&val,4); }
/**
 * ordering files in a directory
 */
static int direntcmp(const void *a, const void *b)
{
    return strcasecmp((char *)((FSZ_DirEnt *)a)->name,(char *)((FSZ_DirEnt *)b)->name);
}
void printf_uuid(FSZ_Access *mem)
{
    printf("%08x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x:",mem->Data1,mem->Data2,mem->Data3,
        mem->Data4[0],mem->Data4[1],mem->Data4[2],mem->Data4[3],mem->Data4[4],mem->Data4[5],mem->Data4[6]);
    printf("%c%c%c%c%c%c%c",
        mem->access&FSZ_READ?'r':'-',
        mem->access&FSZ_WRITE?'w':'-',
        mem->access&FSZ_EXEC?'e':'-',
        mem->access&FSZ_APPEND?'a':'-',
        mem->access&FSZ_DELETE?'d':'-',
        mem->access&FSZ_READ?'u':'-',
        mem->access&FSZ_READ?'s':'-');
}
/* file system functions */
/**
 * add an FS/Z superblock to the output
 */
void add_superblock()
{
    FSZ_SuperBlock *sb;
    fs=realloc(fs,size+secsize);
    if(fs==NULL) exit(4);
    memset(fs+size,0,secsize);
    sb=(FSZ_SuperBlock *)(fs+size);
    memcpy(sb->magic,FSZ_MAGIC,4);
    memcpy((char*)&sb->owner,"root",4);
    sb->version_major=FSZ_VERSION_MAJOR;
    sb->version_minor=FSZ_VERSION_MINOR;
    sb->raidtype=FSZ_SB_SOFTRAID_NONE;
    sb->logsec=secsize==2048?0:(secsize==4096?1:2); //0=2048, 1=4096, 2=8192
    sb->physec=secsize/512; //logsec/mediasectorsize
    sb->maxmounts=255;
    sb->currmounts=0;
    sb->createdate=ts;
    memcpy(sb->magic2,FSZ_MAGIC,4);
    size+=secsize;
}

/**
 * create and append an inode returning it's lsn
 */
int add_inode(char *filetype, char *mimetype)
{
    int i,j=!strcmp(filetype,FILETYPE_SYMLINK)||!strcmp(filetype,FILETYPE_UNION)?secsize-1024:43;
    FSZ_Inode *in;
    fs=realloc(fs,size+secsize);
    if(fs==NULL) exit(4);
    memset(fs+size,0,secsize);
    in=(FSZ_Inode *)(fs+size);
    memcpy(in->magic,FSZ_IN_MAGIC,4);
    memcpy((char*)&in->owner,"root",4);
    in->owner.access=FSZ_READ|FSZ_WRITE|FSZ_DELETE;
    if(filetype!=NULL){
        i=strlen(filetype);
        memcpy(in->filetype,filetype,i>4?4:i);
        if(!strcmp(filetype,FILETYPE_DIR)){
            FSZ_DirEntHeader *hdr=(FSZ_DirEntHeader *)(in->inlinedata);
            in->sec=size/secsize;
            in->flags=FSZ_IN_FLAG_INLINE;
            in->size=sizeof(FSZ_DirEntHeader);
            memcpy(in->inlinedata,FSZ_DIR_MAGIC,4);
            hdr->checksum=crc32_calc((char*)hdr+sizeof(FSZ_DirEntHeader),hdr->numentries*sizeof(FSZ_DirEnt));
        }
    }
    if(mimetype!=NULL){
        if(!strcmp(filetype,FILETYPE_UNION)){
            for(i=1;i<j && !(mimetype[i-1]==0 && mimetype[i]==0);i++);
        } else {
            i=strlen(mimetype);
        }
        memcpy(j==43?in->mimetype:in->inlinedata,mimetype,i>j?j:i);
        if(j!=43)
            in->size=i;
    }
    in->changedate=ts;
    in->modifydate=ts;
    in->checksum=crc32_calc((char*)in->filetype,1016);
    size+=secsize;
    return size/secsize-1;
}

/**
 * register an inode in the directory hierarchy
 */
void link_inode(int inode, char *path, int toinode)
{
    int ns=0,cnt=0;
    FSZ_DirEntHeader *hdr;
    FSZ_DirEnt *ent;
    if(toinode==0)
        toinode=((FSZ_SuperBlock *)fs)->rootdirfid;
    hdr=(FSZ_DirEntHeader *)(fs+toinode*secsize+1024);
    ent=(FSZ_DirEnt *)hdr; ent++;
    while(path[ns]!='/'&&path[ns]!=0) ns++;
    while(ent->fid!=0 && cnt<((secsize-1024)/128-1)) {
        if(!strncmp((char *)(ent->name),path,ns+1)) {
            link_inode(inode,path+ns+1,ent->fid);
            return;
        }
        ent++; cnt++;
    }
    FSZ_Inode *in=((FSZ_Inode *)(fs+toinode*secsize));
    FSZ_Inode *in2=((FSZ_Inode *)(fs+inode*secsize));
    ent->fid=inode;
    ent->length=strlen(path);
    memcpy(ent->name,path,strlen(path));
    if(!strncmp((char *)(((FSZ_Inode *)(fs+inode*secsize))->filetype),"dir:",4)){
        ent->name[ent->length++]='/';
    }
    hdr->numentries++;
    in->modifydate=ts;
    in->size+=sizeof(FSZ_DirEnt);
    qsort((char*)hdr+sizeof(FSZ_DirEntHeader), hdr->numentries, sizeof(FSZ_DirEnt), direntcmp);
    hdr->checksum=crc32_calc((char*)hdr+sizeof(FSZ_DirEntHeader),hdr->numentries*sizeof(FSZ_DirEnt));
    in->checksum=crc32_calc((char*)in->filetype,1016);
    in2->numlinks++;
    in2->checksum=crc32_calc((char*)in2->filetype,1016);
}

/**
 * read a file and add it to the output
 */
void add_file(char *name, char *datafile)
{
    FSZ_Inode *in;
    char *data=readfileall(datafile);
    long int i,j,k,s=((read_size+secsize-1)/secsize)*secsize;
    int inode=add_inode("application","octet-stream");
    fs=realloc(fs,size+s+secsize);
    if(fs==NULL) exit(4);
    memset(fs+size,0,s+secsize);
    in=(FSZ_Inode *)(fs+inode*secsize);
    in->changedate=ts;
    in->modifydate=ts;
    in->size=read_size;
    if(read_size<=secsize-1024) {
        // small, inline data
        in->sec=inode;
        in->flags=FSZ_IN_FLAG_INLINE;
        in->numblocks=0;
        memcpy(in->inlinedata,data,read_size);
        s=0;
    } else {
        in->sec=size/secsize;
        if(read_size>secsize) {
            char *ptr;
            // sector directory
            ptr=fs+size; j=s/secsize;
            in->numblocks=1;
            if(j*8>secsize){
                fprintf(stderr,"File too big: %s\n",datafile);
                exit(5);
            }
            k=inode+2;
            for(i=0;i<j;i++){
                // no spare files (holes) for initrd, as core/fs.c maps the
                // files on it as-is, and we will gzip the initrd anyway
                if(initrd || memcmp(data+i*secsize,emptysec,secsize)) {
                    memcpy(ptr,&k,4);
                    memcpy(fs+size+(i+1)*secsize,data+i*secsize,
                        (i+1)*secsize>read_size?read_size%secsize:secsize);
                    k++;
                    in->numblocks++;
                } else {
                    s-=secsize;
                }
                ptr+=16;
            }
            size+=secsize;
            in->flags=FSZ_IN_FLAG_SD;
        } else {
            // direct sector link
            in->flags=FSZ_IN_FLAG_DIRECT;
            if(memcmp(data,emptysec,secsize)) {
                in->numblocks=1;
                memcpy(fs+size,data,read_size);
            } else {
                in->sec=0;
                in->numblocks=0;
            }
        }
    }
    // detect file type
    if(!strncmp(data+1,"ELF",3))
        {memset(in->mimetype,0,44);memcpy(in->mimetype,"executable",10);in->owner.access|=FSZ_EXEC;}
    if(!strcmp(name+strlen(name)-3,".so"))
        {memset(in->mimetype,0,44);memcpy(in->mimetype,"sharedlib",9);}
    else
    // this is a simple creator, should use libmagic. But this
    // is enough for our purposes, so don't introduce a dependency
    if(!strcmp(name+strlen(name)-2,".h")||
       !strcmp(name+strlen(name)-2,".c")||
       !strcmp(name+strlen(name)-3,".md")||
       !strcmp(name+strlen(name)-4,".txt")||
       !strcmp(name+strlen(name)-5,".conf")
      ) {memset(in->mimetype,0,44);memcpy(in->mimetype,"plain",5);
         memcpy(in->filetype,"text",4);
        }
    else
    if(!strcmp(name+strlen(name)-4,".htm")||
       !strcmp(name+strlen(name)-5,".html")
      )
        {memset(in->mimetype,0,44);memcpy(in->mimetype,"html",4);
         memcpy(in->filetype,"text",4);
        }
    else
    if(!strcmp(name+strlen(name)-4,".svg"))
        {memset(in->mimetype,0,44);memcpy(in->mimetype,"svg",3);
         memcpy(in->filetype,"imag",4);
        }
    else
    if(!strcmp(name+strlen(name)-4,".gif"))
        {memset(in->mimetype,0,44);memcpy(in->mimetype,"gif",3);
         memcpy(in->filetype,"imag",4);
        }
    else
    if(!strcmp(name+strlen(name)-4,".png"))
        {memset(in->mimetype,0,44);memcpy(in->mimetype,"png",3);
         memcpy(in->filetype,"imag",4);
        }
    else
    if(!strcmp(name+strlen(name)-4,".jpg"))
        {memset(in->mimetype,0,44);memcpy(in->mimetype,"jpeg",4);
         memcpy(in->filetype,"imag",4);
        }
    else
    if(!strcmp(name+strlen(name)-4,".bmp"))
        {memset(in->mimetype,0,44);memcpy(in->mimetype,"bitmap",6);
         memcpy(in->filetype,"imag",4);
        }
    else {
        // detect plain 7bit ascii files
        j=1; for(i=0;i<read_size;i++) if(data[i]<7||data[i]>=127) { j=0; break; }
        if(j) {
         memset(in->mimetype,0,44);memcpy(in->mimetype,"plain",5);
         memcpy(in->filetype,"text",4);
        }
    }
    in->checksum=crc32_calc((char*)in->filetype,1016);
    size+=s;
    link_inode(inode,name,0);
}

/**
 * recursively add a directory to the output
 */
void add_dirs(char *dirname,int parent,int level)
{
    DIR *dir;
    struct dirent *ent;
    if(level>4) return;
    char *full=malloc(4096);
    char *ptrto=malloc(secsize-1024);
    if(parent==0) parent=strlen(dirname)+1;
    if ((dir = opendir (dirname)) != NULL) {
      while ((ent = readdir (dir)) != NULL) {
        if(!strcmp(ent->d_name,".")||!strcmp(ent->d_name,".."))
            continue;
        sprintf(full,"%s/%s",dirname,ent->d_name);
        if(ent->d_type==DT_DIR) {
            // recursively add directory structure
            int i=add_inode("dir:",NULL);
            link_inode(i,full+parent,0);
            add_dirs(full,parent,level+1);
        }
        if(ent->d_type==DT_REG) {
            // add a plain file
            add_file(full+parent,full);
        }
        if(ent->d_type==DT_LNK) {
            // add a symbolic link
            int s=readlink(full,ptrto,FSZ_SECSIZE-1024-1); ptrto[s]=0;
            int i=add_inode(FILETYPE_SYMLINK,ptrto);
            link_inode(i,full+parent,0);
        }
        // maybe add other types, blkdev, socket etc.
      }
      closedir (dir);
    }
}

/* functions for the mkfs tool */
/**
 * locate a file inside an FS/Z image
 */
FSZ_Inode *locate(char *data, int inode, char *path)
{
    int ns=0,cnt=0;
    FSZ_DirEntHeader *hdr;
    FSZ_DirEnt *ent;
    hdr=(FSZ_DirEntHeader *)(data+(inode?inode:((FSZ_SuperBlock *)data)->rootdirfid)*secsize+1024);
    ent=(FSZ_DirEnt *)hdr; ent++;
    if(path==NULL || path[0]==0)
        return (FSZ_Inode *)(data + inode*secsize);
    if(path[0]=='/') {
        path++;
        if(path[0]==0)
            return (FSZ_Inode *)(data + (((FSZ_SuperBlock *)data)->rootdirfid)*secsize);
    }
    while(path[ns]!='/'&&path[ns]!=0) ns++;
    while(ent->fid!=0 && cnt<((secsize-1024)/128-1)) {
        if(!strncmp((char *)(ent->name),path,ns+1)) {
            if(path[ns]==0)
                return (FSZ_Inode *)(data + ent->fid*secsize);
            else
                return locate(data,ent->fid,path+ns+1);
        }
        if(!strncmp((char *)(ent->name),path,ns)&&path[ns]==0&&ent->name[ns]=='/') {
            return (FSZ_Inode *)(data + ent->fid*secsize);
        }
        ent++; cnt++;
    }
    return NULL;
}

/**
 * packed check
 */
void checkcompilation()
{
    //Self tests.
    FSZ_SuperBlock sb;
    FSZ_Inode in;
    FSZ_DirEntHeader hdr;
    FSZ_DirEnt ent;
    // ********* WARNING *********
    // These numbers MUST match the ones in: etc/include/fsZ.h
    if( (uint64_t)(&sb.numsec) - (uint64_t)(&sb) != 528 ||
        (uint64_t)(&sb.rootdirfid) - (uint64_t)(&sb) != 560 ||
        (uint64_t)(&sb.owner) - (uint64_t)(&sb) != 744 ||
        (uint64_t)(&sb.magic2) - (uint64_t)(&sb) != 1016 ||
        (uint64_t)(&in.filetype) - (uint64_t)(&in) != 8 ||
        (uint64_t)(&in.version5) - (uint64_t)(&in) != 128 ||
        (uint64_t)(&in.sec) - (uint64_t)(&in) != 448 ||
        (uint64_t)(&in.size) - (uint64_t)(&in) != 464 ||
        (uint64_t)(&in.groups) - (uint64_t)(&in) != 512 ||
        ((uint64_t)(&in.inlinedata) - (uint64_t)(&in) != 1024 &&
         (uint64_t)(&in.inlinedata) - (uint64_t)(&in) != 2048) ||
        (uint64_t)(&hdr.numentries) - (uint64_t)(&hdr) != 16 ||
        (uint64_t)(&ent.name) - (uint64_t)(&ent) != 17) {
        fprintf(stderr,"mkfs: your compiler rearranged structure members. Recompile me with packed struct.\n");
        exit(1);
    }
}

/* command line interface routines */
/**
 * assemble all together. Get the partition images and write out as a whole disk
 */
int createdisk()
{
    unsigned long int i,j=0,gs=7*512,rs=0,es,us,vs,hs,ss=4096;
    unsigned long int uuid[4]={0x12345678,0x12345678,0x12345678,0x12345678};
    char *esp=readfileall(espfile);
    es=read_size;
    char *ssp=readfileall(sysfile);
    rs=read_size;
    char *usr=readfileall(usrfile);
    us=read_size;
    char *var=readfileall(varfile);
    vs=read_size;
    char *home=readfileall(homefile);
    hs=read_size;
    char *gpt=malloc(gs+512),*p;
    char *swap=malloc(ss);
    // get MBR / VBR code if any
    char *loader=readfileall(stage1);   //stage1 loader
    if(loader==NULL) {
        loader=malloc(512);
        memset(loader,0,512);
    } else {
        memset(loader+0x1B8,0,0x1FE - 0x1B8);
    }
    j=0;
    // locate stage2 loader FS0:\BOOTBOOT\LOADER on ESP
    if(es>0) {
        for(i=0;i<es-512;i+=512) {
            if((unsigned char)esp[i+0]==0x55 &&
               (unsigned char)esp[i+1]==0xAA &&
               (unsigned char)esp[i+3]==0xE9 &&
               (unsigned char)esp[i+8]=='B' &&
               (unsigned char)esp[i+12]=='B') {
                // save stage2 address in stage1
                setint(((i+gs)/512)+1,loader+0x1B0);
                j=1;
                break;
            }
        }
    }
    // locate stage2 loader /sys/loader on root partition
    if(!j && rs>0) {
        for(i=0;i<rs-512;i+=512) {
            if((unsigned char)ssp[i+0]==0x55 &&
               (unsigned char)ssp[i+1]==0xAA &&
               (unsigned char)ssp[i+3]==0xE9 &&
               (unsigned char)ssp[i+8]=='B' &&
               (unsigned char)ssp[i+12]=='B') {
                // save stage2 address in stage1
                setint(((i+gs+es)/512)+1,loader+0x1B0);
                break;
            }
        }
    }
    // magic
    loader[0x1FE]=0x55; loader[0x1FF]=0xAA;

    // copy stage1 loader to VBR too. This will be used when
    // later a boot manager is installed on disk
    if(loader[0]!=0) {
        memcpy(rs>0?ssp:usr, loader, 512);
    }

    // WinNT disk id
    setint(uuid[0],loader+0x1B8);

    // generate partitioning tables
    
    // NOTE: on systems that does not support GPT, OS/Z uses MBR types 0x30-0x33
    // 1st part: hardware dependent (most likely 0xC, /boot)
    // 2nd part: OS/Z system (0x30, /usr)
    // 3rd part: OS/Z public (0x31, /var)
    // 4th part: OS/Z private (0x32, /home)

    // MBR, GPT record
    setint(1,loader+0x1C0);                     //start CHS
    loader[0x1C2]=0xEE;                         //type
    setint((gs/512)+1,loader+0x1C4);            //end CHS
    setint(1,loader+0x1C6);                     //start lba
    setint((gs/512),loader+0x1CA);              //number of sectors
    j=0x1D0;
    // MBR, EFI System Partition record
    if(es>0) {
        setint((gs/512)+2,loader+j);            //start CHS
        loader[j+2]=0xEF;                       //type
        setint(((gs+es)/512)+2,loader+j+4);     //end CHS
        setint((gs/512)+1,loader+j+6);          //start lba
        setint(((es)/512),loader+j+10);         //number of sectors
        j+=16;
    }
    // MBR, bootable OS/Z System partition (for optional boot manager)
    loader[j-2]=0x80;                           //bootable flag
    setint(((gs+es)/512)+3,loader+j);           //start CHS
    loader[j+2]=0x30;                           //type
    setint(((gs+es)/512)+3+512,loader+j+4);     //end CHS
    setint(((gs+es+rs)/512)+1,loader+j+6);      //start lba
    setint(((us)/512),loader+j+10);             //number of sectors

    // GPT Header
    memset(gpt,0,gs);
    memcpy(gpt,"EFI PART",8);                       //magic
    setint(1,gpt+10);                               //revision
    setint(92,gpt+12);                              //size
    setint(0xDEADCC32,gpt+16);                      //crc
    setint(1,gpt+24);                               //primarylba
    setint(((gs+gs+es+rs+us+vs+hs+ss)/512),gpt+32); //backuplba
    setint((gs/512)+1,gpt+40);                      //firstusable
    setint(((gs+es+rs+us+vs+hs+ss)/512),gpt+48);    //lastusable
    setint(uuid[0],gpt+56);                         //disk UUID
    setint(uuid[1],gpt+60);
    setint(uuid[2],gpt+64);
    setint(uuid[3],gpt+68);
    setint(2,gpt+72);                               //partitionlba
    setint(3+(es?1:0)+(rs?1:0),gpt+80);             //numentries
    setint(128,gpt+84);                             //entrysize
    setint(0xDEADCC32,gpt+88);                      //entriescrc

    p=gpt+512;
    // GPT, EFI System Partition (mounted at /boot) OPTIONAL
    if(es>0) {
        setint(0x0C12A7328,p);                       //entrytype
        setint(0x011D2F81F,p+4);
        setint(0x0A0004BBA,p+8);
        setint(0x03BC93EC9,p+12);
        setint(uuid[0]+1,p+16);                      //partition UUID
        setint(uuid[1],p+20);
        setint(uuid[2],p+24);
        setint(uuid[3],p+28);
        setint((gs/512)+1,p+32);                     //startlba
        setint(((gs+es)/512),p+40);                  //endlba
        memcpy(p+64,L"EFI System Partition",42);     //name
        p+=128;
    }

    // GPT, OS/Z System Partition (mounted at /) OPTIONAL MBR type 0x30
    if(rs>0) {
        memcpy(p,"OS/Z",4);                      //entrytype, magic
        setint(256,p+4);                         //version 1.0, no flags
        memcpy(p+12,"root",4);                   //mount point
        setint(uuid[0]+2,p+16);                  //partition UUID
        setint(uuid[1],p+20);
        setint(uuid[2],p+24);
        setint(uuid[3],p+28);
        setint(((gs+es)/512)+1,p+32);            //startlba
        setint(((gs+es+rs)/512),p+40);           //endlba
        setint(4,p+48);                          //bootable flag
        memcpy(p+64,L"OS/Z Root",22);            //name
        p+=128;
    }
    // GPT, OS/Z System Partition (mounted at /usr) MBR type 0x30
    memcpy(p,"OS/Z",4);                          //entrytype, magic
    setint(256,p+4);                             //version 1.0, no flags
    memcpy(p+12,"usr",3);                        //mount point
    setint(uuid[0]+3,p+16);                      //partition UUID
    setint(uuid[1],p+20);
    setint(uuid[2],p+24);
    setint(uuid[3],p+28);
    setint(((gs+es+rs)/512)+1,p+32);             //startlba
    setint(((gs+es+rs+us)/512),p+40);            //endlba
    memcpy(p+64,L"OS/Z System",22);              //name
    p+=128;

    // GPT, OS/Z Public Data Partition (mounted at /var) MBR type 0x31
    memcpy(p,"OS/Z",4);                          //entrytype, magic
    setint(256,p+4);                             //version 1.0, no flags
    memcpy(p+12,"var",3);                        //mount point
    setint(uuid[0]+4,p+16);                      //partition UUID
    setint(uuid[1],p+20);
    setint(uuid[2],p+24);
    setint(uuid[3],p+28);
    setint(((gs+es+rs+us)/512)+1,p+32);          //startlba
    setint(((gs+es+rs+us+vs)/512),p+40);         //endlba
    memcpy(p+64,L"OS/Z Public Data",32);         //name
    p+=128;

    // GPT, OS/Z Home Partition (mounted at /home) MBR type 0x32
    memcpy(p,"OS/Z",4);                          //entrytype, magic
    setint(256,p+4);                             //version 1.0, no flags
    memcpy(p+12,"home",4);                       //mount point
    setint(uuid[0]+5,p+16);                      //partition UUID
    setint(uuid[1],p+20);
    setint(uuid[2],p+24);
    setint(uuid[3],p+28);
    setint(((gs+es+rs+us+vs)/512)+1,p+32);       //startlba
    setint(((gs+es+rs+us+vs+hs)/512),p+40);      //endlba
    memcpy(p+64,L"OS/Z Private Data",34);        //name
    p+=128;

    // GPT, OS/Z Swap Partition (mounted at /dev/swap) MBR type 0x33
    memcpy(p,"OS/Z",4);                          //entrytype, magic
    setint(256,p+4);                             //version 1.0, no flags
    memcpy(p+12,"swap",4);                       //mount point
    setint(uuid[0]+6,p+16);                      //partition UUID
    setint(uuid[1],p+20);
    setint(uuid[2],p+24);
    setint(uuid[3],p+28);
    setint(((gs+es+rs+us+vs+hs)/512)+1,p+32);    //startlba
    setint(((gs+es+rs+us+vs+hs+ss)/512),p+40);   //endlba
    memcpy(p+64,L"OS/Z Swap Area",30);           //name
    p+=128;

    // Checksums
    //partitions
    i=(int)((unsigned char)gpt[80]*(unsigned char)gpt[84]); //length=number*size
    setint(eficrc32_calc(gpt+512,i),gpt+88);
    //header
    i=getint(gpt+12);   //header size
    setint(0,gpt+16);   //must be zero when calculating
    setint(eficrc32_calc(gpt,i),gpt+16);

    f=fopen(diskname,"wb");
    // (P)MBR
    fwrite(loader,512,1,f);
    // GPT header + entries
    fwrite(gpt,gs,1,f);
    // Partitions
    if(es>0)
        fwrite(esp,es,1,f);
    if(rs>0)
        fwrite(ssp,rs,1,f);
    fwrite(usr,us,1,f);
    fwrite(var,vs,1,f);
    fwrite(home,hs,1,f);
    fwrite(swap,ss,1,f);

    //write GPT partition entries again
    fwrite(gpt+512,gs-512,1,f);
    //write GPT backup header
    i=getint(gpt+32);
    setint(getint(gpt+24),gpt+32);                     //backuplba
    setint(i,gpt+24);                                  //primarylba

    setint((i*512-gs)/512+1,gpt+72);                   //partitionlba
    i=getint(gpt+12);   //header size
    setint(0,gpt+16);   //must be zero when calculating
    setint(eficrc32_calc(gpt,i),gpt+16);
    fwrite(gpt,512,1,f);
    fclose(f);
    return 1;
}

/**
 * create an fs image of a directory
 */
int createimage(char *image,char *dir)
{
    // are we creating initrd? Needed by add_file
    initrd=(strlen(image)>7 && !memcmp(image+strlen(image)-6,"INITRD",6));

    fs=malloc(secsize);
    if(fs==NULL) exit(3);
    size=0;
    // superblock
    add_superblock();
    // add root directory
    ((FSZ_SuperBlock *)fs)->rootdirfid = add_inode("dir:",MIMETYPE_DIR_ROOT);
    ((FSZ_Inode*)(fs+secsize))->numlinks++;
    ((FSZ_Inode*)(fs+secsize))->checksum=crc32_calc((char*)(((FSZ_Inode*)(fs+secsize))->filetype),1016);

    // recursively parse the directory and add everything in it
    // into the fs image
    add_dirs(dir,0,0);

    //modify the total number of sectors
    ((FSZ_SuperBlock *)fs)->numsec=size/secsize;
    ((FSZ_SuperBlock *)fs)->freesec=size/secsize;
    ((FSZ_SuperBlock *)fs)->checksum=crc32_calc((char *)((FSZ_SuperBlock *)fs)->magic,508);

    //write out new image
    f=fopen(image,"wb");
    fwrite(fs,size,1,f);
    fclose(f);
    return 1;
}

/**
 * list a directory or union in an image
 */
void ls(int argc, char **argv)
{
    char *data=readfileall(argv[1]);
    FSZ_Inode *dir;
    if(argc<4) {
        dir=(FSZ_Inode*)(data+(((FSZ_SuperBlock *)data)->rootdirfid)*secsize);
    } else
        dir = locate(data,0,argv[3]);

    if(dir==NULL) { printf("mkfs: Unable to find path in image\n"); exit(2); }
    int cnt=0;
    if(!memcmp(dir->filetype,FILETYPE_UNION,4)){
        char *c=((char*)dir+1024);
        printf("Union list of:\n");
        while(*c!=0) {
            printf("  %s\n",c);
            while(*c!=0) c++;
            c++;
        }
        return;
    }
    if(memcmp(dir->filetype,FILETYPE_DIR,4)) { printf("mkfs: not a directory\n"); exit(2); }
    FSZ_DirEntHeader *hdr=(FSZ_DirEntHeader *)((char*)dir+1024);
    FSZ_DirEnt *ent;
    ent=(FSZ_DirEnt *)hdr; ent++;
    while(ent->fid!=0 && cnt<((secsize-1024)/128-1)) {
        FSZ_Inode *in = (FSZ_Inode *)((char*)data+ent->fid*secsize);
        printf("  %c%c%c%c %6ld %6ld %s\n",
            in->filetype[0],in->filetype[1],in->filetype[2],in->filetype[3],
            in->sec, in->size, ent->name);
        ent++; cnt++;
    }
}

/**
 * extract a file from an image
 */
void cat(int argc, char **argv)
{
    char *data=readfileall(argv[1]);
    FSZ_Inode *file = locate(data,0,argv[3]);
    if(file==NULL) { printf("mkfs: Unable to find path in image\n"); exit(2); }
    if(size<secsize-1024) {
        fwrite((char*)file + 1024,file->size,1,stdout);
    } else
    if(size<secsize) {
        fwrite(data + file->sec*secsize,file->size,1,stdout);
    } else {
        int i, s=file->size;
        for(i=0;i<file->size/secsize && s>=0;i++) {
            unsigned char *sd = (unsigned char*)(data + file->sec*secsize);
            fwrite(data + ((unsigned int)(*sd))*secsize, s>secsize?secsize:s,1,stdout);
            s-=secsize; sd+=16;
        }
    }
}

/**
 * change the mime type of a file in the image
 */
void changemime(int argc, char **argv)
{
    char *data=readfileall(argv[1]);
    FSZ_Inode *in=NULL;
    if(argc>=4)
        in = locate(data,0,argv[3]);
    if(in==NULL) { printf("mkfs: Unable to find path in image\n"); exit(2); }
    if(argc<5) {
        printf("%c%c%c%c/%s\n",
            in->filetype[0],in->filetype[1],in->filetype[2],in->filetype[3],in->mimetype);
    } else {
        char *c=argv[4];
        while(*c!=0&&*c!='/')c++;
        if(in->filetype[3]==':') { printf("mkfs: Unable to change mime type of special inode\n"); exit(2); }
        if(*c!='/') { printf("mkfs: bad mime type\n"); exit(2); } else c++;
        memcpy(in->filetype,argv[4],4);
        memcpy(in->mimetype,c,strlen(c)<44?strlen(c):43);
        in->checksum=crc32_calc((char*)in->filetype,1016);
        //write out new image
        f=fopen(argv[1],"wb");
        fwrite(data,read_size,1,f);
        fclose(f);
    }
}

/**
 * add an union directory to the image
 */
void addunion(int argc, char **argv)
{
    int i;
    char items[secsize-1024];
    char *c=(char*)&items;
    memset(&items,0,sizeof(items));
    for(i=4;i<argc;i++) {
        memcpy(c,argv[i],strlen(argv[i]));
        c+=strlen(argv[i])+1;
    }
    fs=readfileall(argv[1]); size=read_size;
    i=add_inode(FILETYPE_UNION,(char*)&items);
    memset(&items,0,sizeof(items));
    memcpy(&items,argv[3],strlen(argv[3]));
    if(items[strlen(argv[3])-1]!='/')
        items[strlen(argv[3])]='/';
    link_inode(i,(char*)&items,0);
    //write out new image
    f=fopen(argv[1],"wb");
    fwrite(fs,size,1,f);
    fclose(f);
}

/**
 * add a symbolic link to the output
 */
void addsymlink(int argc, char **argv)
{
    fs=readfileall(argv[1]); size=read_size;
    int i=add_inode(FILETYPE_SYMLINK,argv[4]);
    link_inode(i,argv[3],0);
    //write out new image
    f=fopen(argv[1],"wb");
    fwrite(fs,size,1,f);
    fclose(f);
}

/**
 * create an Option ROM out of an image for in-ROM initrd
 */
void initrdrom(int argc, char **argv)
{
    int i;
    unsigned char *buf, c=0;
    fs=readfileall(argv[1]); size=((read_size+32+511)/512)*512;
    buf=(unsigned char*)malloc(size+1);
    //Option ROM Header
    buf[0]=0x55; buf[1]=0xAA; buf[2]=(read_size+32+511)/512;
    // asm "xor ax,ax; retf"
    buf[3]=0x31; buf[4]=0xC0; buf[5]=0xCB;
    //magic, size and data
    memcpy(buf+8,"INITRD",6);
    memcpy(buf+16,&read_size,4);
    memcpy(buf+32,fs,read_size);
    //checksum
    for(i=0;i<size;i++) c+=buf[i];
    buf[6]=(unsigned char)((int)(256-c));
    //write out new image
    f=fopen(argc>2&&argv[3]!=NULL?argv[3]:"initrd.rom","wb");
    fwrite(buf,size,1,f);
    fclose(f);
}

/**
 * dump sectors inside an image into C structs
 */
void dump(int argc, char **argv)
{
    int i=argv[3]!=NULL?atoi(argv[3]):0;
    uint64_t j,*k,l,secs=secsize;
    char unit='k',*c;
    fs=readfileall(argv[1]);
    FSZ_SuperBlock *sb=(FSZ_SuperBlock *)fs;
    FSZ_DirEntHeader *hdr=(FSZ_DirEntHeader*)(fs+i*secsize);
    FSZ_DirEnt *d;
    if(memcmp(&sb->magic, FSZ_MAGIC, 4) || memcmp(&sb->magic2, FSZ_MAGIC, 4)) {
        printf("mkfs: Not an FS/Z image, bad magic. Compressed?\n"); exit(2);
    }
    secs=1<<(sb->logsec+11);
    if(i*secs>read_size) { printf("mkfs: Unable to allocate %d sector\n",i); exit(2); }
    printf("  ---------------- Dumping sector #%d ----------------\n",i);
    if(i==0) {
        printf("FSZ_SuperBlock {\n");
        printf("\tmagic: %s\n",FSZ_MAGIC);
        printf("\tversion_major.version_minor: %d.%d\n",sb->version_major,sb->version_minor);
        printf("\tflags: 0x%02x %s %s\n",sb->flags,sb->flags&FSZ_SB_FLAG_HIST?"FSZ_SB_FLAG_HIST":"",
            sb->flags&FSZ_SB_FLAG_BIGINODE?"FSZ_SB_FLAG_BIGINODE":"");
        printf("\traidtype: 0x%02x %s\n",sb->raidtype,sb->raidtype==FSZ_SB_SOFTRAID_NONE?"none":"");
        printf("\tlogsec: %d (%ld bytes per sector)\n",sb->logsec,secs);
        printf("\tphysec: %d (%ld bytes per sector)\n",sb->physec,secs/sb->physec);
        printf("\tcurrmounts / maxmounts: %d / %d\n",sb->currmounts,sb->maxmounts);
        j=sb->numsec*(1<<(sb->logsec+11))/1024;
        if(j>1024) { j/=1024; unit='M'; } printf("\tnumsec: %ld (total capacity %ld %cbytes)\n",sb->numsec,j,unit); 
        printf("\tfreesec: %ld (%ld%% free, not including free holes)\n",sb->freesec,
            (sb->numsec-sb->freesec)*100/sb->numsec);
        printf("\trootdirfid: LSN %ld\n",sb->rootdirfid);
        printf("\tfreesecfid: LSN %ld %s\n",sb->freesecfid,sb->freesecfid==0?"(no free holes)":"");
        printf("\tbadsecfid: LSN %ld\n",sb->badsecfid);
        printf("\tindexfid: LSN %ld\n",sb->indexfid);
        printf("\tmetafid: LSN %ld\n",sb->metafid);
        printf("\tjournalfid: LSN %ld (start %ld, end %ld)\n",sb->journalfid, sb->journalstr, sb->journalend);
        printf("\tenchash: 0x%04x %s\n",sb->enchash, sb->enchash==0?"none":"encrypted");
        j=sb->createdate/1000000; printf("\tcreatedate: %ld %s",sb->createdate,ctime((time_t*)&j));
        j=sb->lastmountdate/1000000; printf("\tlastmountdate: %ld %s",sb->lastmountdate,ctime((time_t*)&j));
        j=sb->lastcheckdate/1000000; printf("\tlastcheckdate: %ld %s",sb->lastcheckdate,ctime((time_t*)&j));
        j=sb->lastdefragdate/1000000; printf("\tlastdefragdate: %ld %s",sb->lastmountdate,ctime((time_t*)&j));
        printf("\towner: "); printf_uuid((FSZ_Access*)&sb->owner); printf("\n");
        printf("\tmagic2: %s\n",FSZ_MAGIC);
        printf("\tchecksum: 0x%04x %s\n",sb->checksum,
            sb->checksum==crc32_calc((char *)sb->magic,508)?"correct":"invalid");
        printf("\traidspecific: %s\n",sb->raidspecific);
        printf("};\n");
    } else
    if(!memcmp(fs+i*secs,FSZ_IN_MAGIC,4)) {
        FSZ_Inode *in=(FSZ_Inode*)(fs+i*secs);
        printf("FSZ_Inode {\n");
        printf("\tmagic: %s\n",FSZ_IN_MAGIC);
        printf("\tchecksum: 0x%04x %s\n",in->checksum,
            in->checksum==crc32_calc((char*)in->filetype,1016)?"correct":"invalid");
        printf("\tfiletype: \"%c%c%c%c\", mimetype: \"%s\"\n",in->filetype[0],in->filetype[1],
            in->filetype[2],in->filetype[3],in->mimetype);
        printf("\tenchash: 0x%04x %s\n",in->enchash, in->enchash==0?"none":"encrypted");
        j=in->changedate/1000000; printf("\tchangedate: %ld %s",in->changedate,ctime((time_t*)&j));
        j=in->accessdate/1000000; printf("\taccessdate: %ld %s",in->accessdate,ctime((time_t*)&j));
        printf("\tnumblocks: %ld\n\tnumlinks: %ld\n",in->numblocks,in->numlinks);
        printf("\tmetalabel: LSN %ld\n",in->metalabel);
        printf("\tFSZ_Version {\n");
        printf("\t\tsec: LSN %ld %s\n",in->sec,in->sec==0?"spare":(in->sec==i?"self-reference (inlined)":""));
        printf("\t\tsize: %ld bytes\n",in->size);
        j=in->modifydate/1000000; printf("\t\tmodifydate: %ld %s",in->modifydate,ctime((time_t*)&j));
        printf("\t\tfilechksum: 0x%04x %s\n",in->filechksum,in->filechksum==0?"not implemented":"");
        printf("\t\tflags: 0x%04x ",in->flags);
        switch(FLAG_TRANSLATION(in->flags)){
            case FSZ_IN_FLAG_INLINE: printf("FSZ_IN_FLAG_INLINE"); break;
            case FSZ_IN_FLAG_DIRECT: printf("FSZ_IN_FLAG_DIRECT"); break;
            case FSZ_IN_FLAG_SECLIST: printf("FSZ_IN_FLAG_SECLIST"); break;
            case FSZ_IN_FLAG_SECLIST0: printf("FSZ_IN_FLAG_SECLIST0"); break;
            case FSZ_IN_FLAG_SECLIST1: printf("FSZ_IN_FLAG_SECLIST1"); break;
            case FSZ_IN_FLAG_SECLIST2: printf("FSZ_IN_FLAG_SECLIST2"); break;
            case FSZ_IN_FLAG_SECLIST3: printf("FSZ_IN_FLAG_SECLIST3"); break;
            case FSZ_IN_FLAG_SD: printf("FSZ_IN_FLAG_SD"); break;
            default: printf("FSZ_IN_FLAG_SD%d",FLAG_TRANSLATION(in->flags)); break;
        }
        printf("\n\t};\n");
        printf("\towner: "); printf_uuid(&in->owner);
        printf("\n\tinlinedata: ");
        if(!memcmp(in->filetype,FILETYPE_SYMLINK,4))
            printf(" symlink target\n\t  '%s'",in->inlinedata);
        else
        if(!memcmp(in->filetype,FILETYPE_UNION,4)) {
            printf("union list\n");
            c=(char*)in->inlinedata;
            while(*c!=0) {
                printf("\t  '%s'\n",c);
                while(*c!=0) c++;
                c++;
            }
        } else
        if(!memcmp(in->inlinedata,FSZ_DIR_MAGIC,4))
            printf("directory entries\n");
        else {
            printf("\n");
            for(j=0;j<8;j++) {
                printf("\t  ");
                for(l=0;l<16;l++) printf("%02x %s",in->inlinedata[j*16+l],l%4==3?" ":"");
                for(l=0;l<16;l++) printf("%c",in->inlinedata[j*16+l]>=30&&
                    in->inlinedata[j*16+l]<127?in->inlinedata[j*16+l]:'.');
                printf("\n");
            }
            printf("\t  ... etc.\n");
        }
        printf("\n};\n");
        printf("Data sectors: ");
        switch(FLAG_TRANSLATION(in->flags)){
            case FSZ_IN_FLAG_INLINE: printf("%ld", in->sec); break;
            case FSZ_IN_FLAG_DIRECT: printf("%ld", in->sec); break;
            case FSZ_IN_FLAG_SECLIST: printf("sl"); break;
            case FSZ_IN_FLAG_SECLIST0: printf("%ld sl", in->sec); break;
            case FSZ_IN_FLAG_SECLIST1: printf("sd %ld [ sl ]", in->sec); break;
            case FSZ_IN_FLAG_SECLIST2: printf("sd %ld [ sd [ sl ]]", in->sec); break;
            case FSZ_IN_FLAG_SECLIST3: printf("sd %ld [ sd [ sd [ sl ]]]", in->sec); break;
            default:
                printf("sd %ld [", in->sec);
                k=(uint64_t*)(fs+in->sec*secs);
                l=secs/16; while(l>1 && k[l*2-1]==0 && k[l*2-2]==0) l--;
                for(j=0;j<l;j++) {
                    if(FLAG_TRANSLATION(in->flags)==FSZ_IN_FLAG_SD)
                        printf(" %ld",*k);
                    else
                        printf(" sd %ld [...]",*k);
                    k+=2;
                }
                printf(" ]");
                break;
        }
        printf("\n");
        if(!memcmp(in->inlinedata,FSZ_DIR_MAGIC,4)) {
            hdr=(FSZ_DirEntHeader*)in->inlinedata;
            printf("\n");
            goto dumpdir;
        }
    } else
    if(!memcmp(fs+i*secs,FSZ_DIR_MAGIC,4)) {
dumpdir:
        d=(FSZ_DirEnt *)hdr;
        printf("FSZ_DirEntHeader {\n");
        printf("\tmagic: %s\n",FSZ_DIR_MAGIC);
        printf("\tchecksum: %04x\n",hdr->checksum);
        printf("\tflags: 0x%04lx %s %s\n",hdr->flags,hdr->flags&FSZ_DIR_FLAG_UNSORTED?"FSZ_DIR_FLAG_UNSORTED":"",
            hdr->flags&FSZ_DIR_FLAG_HASHED?"FSZ_DIR_FLAG_HASHED":"");
        printf("\tnumentries: %ld\n",hdr->numentries);
        printf("};\n");
        for(j=0;j<hdr->numentries;j++) {
            d++;
            printf("FSZ_DirEnt {\n");
            printf("\tfid: LSN %ld\n",d->fid);
            printf("\tlength: %d unicode characters\n",d->length);
            printf("\tname: \"%s\"\n",d->name);
            printf("};\n");
        }
    } else {
        k=(uint64_t*)(fs+i*secs); l=1;
        for(j=0;j<16;j++) {
            k++;
            if(*k!=0) { l=0; break; }
            k++;
        }
        if(l) {
            printf("Probably Sector Directory or Sector List\n");
            k=(uint64_t*)(fs+i*secs);
            l=secs/32; while(l>1 && k[l*4-1]==0 && k[l*4-2]==0 && k[l*4-3]==0 && k[l*4-4]==0) l--;
            for(j=0;j<l;j++) {
                printf("\tLSN %ld\t\t(LSN/size) %ld%s",*k,*(k+2),j%2==0?"\t\t":"\n");
                k+=4;
            }
            printf("\n");
        } else {
            printf("File data\n");
            c=(char*)(fs+i*secs);
            for(j=0;j<8;j++) {
                printf("  ");
                for(l=0;l<16;l++) printf("%02x %s",(unsigned char)(c[j*16+l]),l%4==3?" ":"");
                for(l=0;l<16;l++) printf("%c",(unsigned char)(c[j*16+l]>=30&&c[j*16+l]<127?c[j*16+l]:'.'));
                printf("\n");
            }
            printf("  ... etc.\n");
        }
    }
}

/**
 * main entry point
 */
int main(int argc, char **argv)
{
    char *path=strdup(argv[0]);
    int i;
    // get current timestamp in microsec
    ts = (long int)time(NULL) * 1000000;

    checkcompilation();

    //get filenames
    for(i=strlen(path);i>0;i--) {if(path[i-1]=='/') break;}
    memcpy(path+i,"../bin/",8); i+=8; path[i]=0;
    stage1=malloc(i+16); sprintf(stage1,"%s/../loader/boot.bin",path);
    sysfile=malloc(i+16); sprintf(sysfile,"%ssys.part",path);
    espfile=malloc(i+16); sprintf(espfile,"%sesp.part",path);
    usrfile=malloc(i+16); sprintf(usrfile,"%susr.part",path);
    varfile=malloc(i+16); sprintf(varfile,"%svar.part",path);
    homefile=malloc(i+16); sprintf(homefile,"%shome.part",path);

    // create an empty sector
    emptysec=(char*)malloc(secsize);
    if(emptysec==NULL) { printf("mkfs: Unable to allocate %d memory\n",secsize); exit(1); }
    memset(emptysec,0,secsize);

    //parse arguments
    if(argv[1]==NULL||!strcmp(argv[1],"help")) {
        printf("FS/Z mkfs utility - Copyright 2017 CC-by-nc-sa-4.0 bztsrc@github\n\n"
            "./mkfs (imagetocreate) (createfromdir)         - create a new FS/Z image file\n"
            "./mkfs (imagetoread) initrdrom (romfile)       - create Option ROM from file\n"
            "./mkfs (imagetoread) union (path) (members...) - add an union directory to image file\n"
            "./mkfs (imagetoread) symlink (path) (target)   - add a symbolic link to image file\n"
            "./mkfs (imagetoread) mime (path) (mimetype)    - change the mime type of a file in image file\n"
            "./mkfs (imagetoread) ls (path)                 - parse FS/Z image and list contents\n"
            "./mkfs (imagetoread) cat (path)                - parse FS/Z image and return file content\n"
            "./mkfs (imagetoread) dump (lsn)                - dump a sector in image into C header format\n"
            "./mkfs disk                                    - assemble OS/Z partition images into one GPT disk\n"
            "\nNOTE the FS/Z on disk format is Public Domain, but this utility is not.\n");
        exit(0);
    }
    if(!strcmp(argv[1],"disk")) {
        diskname=malloc(i+64); sprintf(diskname,"%s%s",path,argv[2]!=NULL?argv[2]:"disk.dd");
        createdisk();
    } else
    if(argc>1){
        if(!strcmp(argv[2],"initrdrom")) {
            initrdrom(argc,argv);
        } else
        if(!strcmp(argv[2],"union") && argc>3) {
            addunion(argc,argv);
        } else
        if(!strcmp(argv[2],"symlink") && argc>3) {
            addsymlink(argc,argv);
        } else
        if(!strcmp(argv[2],"mime") && argc>3) {
            changemime(argc,argv);
        } else
        if(!strcmp(argv[2],"cat") && argc>2) {
            cat(argc,argv);
        } else
        if(!strcmp(argv[2],"ls") && argc>=2) {
            ls(argc,argv);
        } else
        if(!strcmp(argv[2],"dump") && argc>=2) {
            dump(argc,argv);
        } else
        if(argc>1) {
            createimage(argv[1],argv[2]);
        } else {
            printf("mkfs: unknown command.\n");
        }
    }
    return 0;
}
