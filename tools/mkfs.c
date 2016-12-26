/*
 * tools/mkfs.c
 * 
 * Copyright 2016 CC-by-nc-sa-4.0 bztsrc@github
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
 *  Usage:
 *  ./mkfs create (file) (dir) - creates a new FS/Z image file
 *  ./mkfs disk - assembles partition images into one GPT disk
 *
 * It's a minimal implementation, has several limitations compared to
 * the FS/Z spec. For example it supports only 24 directory entries
 * per directories (entries are inlined in inode).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include "../etc/include/fsZ.h"
#include "../etc/include/crc32.h"

//-------------DATA-----------
char *diskname;     //entire disk
char *stage1;       //mbr code
char *espfile;      //EFI System Partition imagefile
char *usrfile;      //usr partition imagefile
char *varfile;      //var partition imagefile
char *homefile;     //home partition imagefile

unsigned int secsize=FSZ_SECSIZE;
FILE *f;
char *fs;
int size;
long int li=0,read_size=0;

//-------------CODE-----------

/* Misc functions */
//reads a file into memory
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
//gets or sets integer values in a char array
int getint(char *ptr) { return (unsigned char)ptr[0]+(unsigned char)ptr[1]*256+(unsigned char)ptr[2]*256*256+ptr[3]*256*256*256; }
void setint(int val, char *ptr) { memcpy(ptr,&val,4); }
//orders files in a directory
static int direntcmp(const void *a, const void *b)
{
    return strcasecmp((char *)((FSZ_DirEnt *)a)->name,(char *)((FSZ_DirEnt *)b)->name);
}

//adds a superblock to the output
void add_superblock()
{
    FSZ_SuperBlock *sb;
    fs=realloc(fs,size+secsize);
    if(fs==NULL) exit(4);
    memset(fs+size,0,secsize);
    sb=(FSZ_SuperBlock *)(fs+size);
    memcpy(sb->magic,FSZ_MAGIC,4);
    sb->version_major=FSZ_VERSION_MAJOR;
    sb->version_minor=FSZ_VERSION_MINOR;
    sb->logsec=secsize==2048?0:(secsize==4096?1:2); //0=2048, 1=4096, 2=8192
    sb->physec=secsize/512; //logsec/mediasectorsize
    sb->maxmounts=255;
    sb->currmounts=0;
    memcpy(sb->magic2,FSZ_MAGIC,4);
    sb->checksum=crc32_calc((char *)sb->magic,180);
    size+=secsize;
}

//appends an inode
int add_inode(char *filetype, char *mimetype)
{
    int i,j=!strcmp(filetype,"url:")?secsize-1024:52;
    FSZ_Inode *in;
    fs=realloc(fs,size+secsize);
    if(fs==NULL) exit(4);
    memset(fs+size,0,secsize);
    in=(FSZ_Inode *)(fs+size);
    memcpy(in->magic,FSZ_IN_MAGIC,4);
    if(filetype!=NULL){
        i=strlen(filetype);
        memcpy(in->filetype,filetype,i>4?4:i);
        if(!strcmp(filetype,"dir:")){
            FSZ_DirEntHeader *hdr=(FSZ_DirEntHeader *)(in->inlinedata);
            in->sec=size/secsize;
            memcpy(in->inlinedata,FSZ_DIR_MAGIC,4);
            hdr->checksum=crc32_calc((char*)hdr+sizeof(FSZ_DirEntHeader),hdr->numentries*sizeof(FSZ_DirEnt));
        }
    }
    if(mimetype!=NULL){
        i=strlen(mimetype);
        memcpy(j==52?in->mimetype:in->inlinedata,mimetype,i>j?j:i);
    }
    in->checksum=crc32_calc((char*)in->filetype,secsize-8);
    size+=secsize;
    return size/secsize-1;
}

//registers an inode in the directory hierarchy
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
    ent->fid=inode;
    ent->length=strlen(path);
    memcpy(ent->name,path,strlen(path));
    if(!strncmp((char *)(((FSZ_Inode *)(fs+inode*secsize))->filetype),"dir:",4)){
        ent->name[ent->length++]='/';
    }
    hdr->numentries++;
    qsort((char*)hdr+sizeof(FSZ_DirEntHeader), hdr->numentries, sizeof(FSZ_DirEnt), direntcmp);
    hdr->checksum=crc32_calc((char*)hdr+sizeof(FSZ_DirEntHeader),hdr->numentries*sizeof(FSZ_DirEnt));
}

//reads a file and adds it to the output
void add_file(char *name, char *datafile)
{
    FSZ_Inode *in;
    char *data=readfileall(datafile);
    long int i,j,s=((read_size+secsize-1)/secsize)*secsize;
    int inode=add_inode("application","octet-stream");
    fs=realloc(fs,size+s+secsize);
    if(fs==NULL) exit(4);
    in=(FSZ_Inode *)(fs+inode*secsize);
    in->size=read_size;
    if(read_size<=secsize-1024) {
        // small, inline data
        in->sec=inode;
        memcpy(in->inlinedata,data,read_size);
        s=0;
    } else {
        in->sec=size/secsize;
        if(read_size>secsize) {
            char *ptr;
            // sector directory
            memset(fs+size,0,secsize);
            ptr=fs+size; j=(read_size+secsize-1)/secsize;
            if(j*8>secsize){
                fprintf(stderr,"File too big: %s\n",datafile);
                exit(5);
            }
            for(i=inode+2;j>0;j--){
                memcpy(ptr,&i,4);
                i++; ptr+=8;
            }
            size+=secsize;
        }
        // direct sector link
        memset(fs+size+read_size,0,s-read_size);
        memcpy(fs+size,data,read_size);
    }
    // detect file type
    if(!strncmp(data+1,"ELF",3))
        {memset(in->mimetype,0,52);memcpy(in->mimetype,"executable",10);}
    if(!strcmp(name+strlen(name)-3,".so"))
        {memset(in->mimetype,0,52);memcpy(in->mimetype,"sharedlib",9);}
    else
    // this is a simple creator, should use libmagic. But this
    // is enough for our purposes, so don't introduce a dependency
    if(!strcmp(name+strlen(name)-2,".h")||
       !strcmp(name+strlen(name)-2,".c")||
       !strcmp(name+strlen(name)-3,".md")||
       !strcmp(name+strlen(name)-4,".txt")||
       !strcmp(name+strlen(name)-5,".conf")||
       !strcmp(name+strlen(name)-5,"fstab")||
       !strcmp(name+strlen(name)-8,"hostname")||
       !strcmp(name+strlen(name)-7,"profile")||
       !strcmp(name+strlen(name)-7,"release")
      ) {memset(in->mimetype,0,52);memcpy(in->mimetype,"plain",5);
         memcpy(in->filetype,"text",4);
        }
    else
    if(!strcmp(name+strlen(name)-4,".htm")||
       !strcmp(name+strlen(name)-5,".html")
      )
        {memset(in->mimetype,0,52);memcpy(in->mimetype,"html",4);
         memcpy(in->filetype,"text",4);
        }
    else
    if(!strcmp(name+strlen(name)-4,".gif"))
        {memset(in->mimetype,0,52);memcpy(in->mimetype,"gif",3);
         memcpy(in->filetype,"imag",4);
        }
    else
    if(!strcmp(name+strlen(name)-4,".png"))
        {memset(in->mimetype,0,52);memcpy(in->mimetype,"png",3);
         memcpy(in->filetype,"imag",4);
        }
    if(!strcmp(name+strlen(name)-4,".jpg"))
        {memset(in->mimetype,0,52);memcpy(in->mimetype,"jpeg",4);
         memcpy(in->filetype,"imag",4);
        }
    if(!strcmp(name+strlen(name)-4,".bmp"))
        {memset(in->mimetype,0,52);memcpy(in->mimetype,"bitmap",6);
         memcpy(in->filetype,"imag",4);
        }

    in->checksum=crc32_calc((char*)in->filetype,secsize-8);
    size+=s;
    link_inode(inode,name,0);
}

//recursively add a directory to the output
void add_dirs(char *dirname,int parent,int level)
{
    DIR *dir;
    struct dirent *ent;
    if(level>4) return;
    char *full=malloc(4096);
    char *ptrto=malloc(512-128);
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
            int s=readlink(dirname,ptrto,512-128-1); ptrto[s+1]=0;
            int i=add_inode("url:",ptrto);
            link_inode(i,full+parent,0);
        }
        // maybe add other types, blkdev, socket etc.
      }
      closedir (dir);
    }
}

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

//packed check
void checkcompilation()
{
    //Self tests.
    FSZ_SuperBlock sb;
    FSZ_Inode in;
    FSZ_DirEntHeader hdr;
    FSZ_DirEnt ent;

    // ********* WARNING *********
    // These numbers MUST match the ones in: etc/include/fsZ.h
    if( (uint64_t)(&sb.numsec) - (uint64_t)(&sb) != 520 ||
        (uint64_t)(&sb.rootdirfid) - (uint64_t)(&sb) != 552 ||
        (uint64_t)(&in.filetype) - (uint64_t)(&in) != 8 ||
        (uint64_t)(&in.version5) - (uint64_t)(&in) != 128 ||
        (uint64_t)(&in.sec) - (uint64_t)(&in) != 448 ||
        (uint64_t)(&in.size) - (uint64_t)(&in) != 464 ||
        (uint64_t)(&in.groups) - (uint64_t)(&in) != 512 ||
        (uint64_t)(&in.inlinedata) - (uint64_t)(&in) != 1024 ||
        (uint64_t)(&hdr.numentries) - (uint64_t)(&hdr) != 8 ||
        (uint64_t)(&ent.name) - (uint64_t)(&ent) != 17) {
        fprintf(stderr,"mkfs: your compiler rearranged stucture members. Recompile me with packed struct.\n");
        exit(1);
    }
}

/* command line interface routines */
//Assemble all together. Get the partition images and write out as a whole disk
int createdisk()
{
    unsigned long int i,gs=1024+512,es,us,vs,hs;
    unsigned long int uuid[4]={0x12345678,0x12345678,0x12345678,0x12345678};
    char *esp=readfileall(espfile);
    es=read_size;
    char *usr=readfileall(usrfile);
    us=read_size;
    char *var=readfileall(varfile);
    vs=read_size;
    char *home=readfileall(homefile);
    hs=read_size;
    char *gpt=malloc(gs);
    // get MBR code if any
    char *loader=readfileall(stage1);   //stage1 loader
    if(loader==NULL) {
        loader=malloc(512);
        memset(loader,0,512);
    }
    memset(loader+0x1B8,0,0x1FE - 0x1B8);
    // locate stage2 loader FS0:\BOOTBOOT\LOADER on ESP
    for(i=0;i<es-512;i+=512) {
        if((unsigned char)esp[i+0]==0x55 &&
           (unsigned char)esp[i+1]==0xAA &&
           (unsigned char)esp[i+3]==0xE9 &&
           (unsigned char)esp[i+8]=='B' &&
           (unsigned char)esp[i+12]=='B') {
            // save stage2 address in stage1
            setint(((i+gs)/512)+1,loader+0x1B0);
            break;
        }
    }
    // save ESP, we've modified pointers
    f=fopen(espfile,"wb");
    if(f) {
        fwrite(esp,es,1,f);
        fclose(f);
    }

    // WinNT disk id
    setint(uuid[0],loader+0x1B8);

    // generate partitioning tables
    // MBR, GPT record
    loader[0x1C2]=0xEE;                         //type
    setint(1,loader+0x1C6);                     //start lba
    setint((gs/512),loader+0x1CA);              //end lba
    // MBR, EFI System Partition record
    loader[0x1D2]=0xEF;                         //type
    setint((gs/512)+1,loader+0x1D6);            //start lba
    setint(((gs+es)/512),loader+0x1DA);         //end lba
    // MBR, OS/Z System Partition record
    loader[0x1E2]='Z';                          //type
    setint(((gs+es)/512)+1,loader+0x1E6);       //start lba
    setint(((gs+es+us)/512),loader+0x1EA);      //end lba
    // MBR, OS/Z Public Data Partition record
    loader[0x1F2]='Z';                          //type
    setint(((gs+es+us)/512)+1,loader+0x1F6);    //start lba
    setint(((gs+es+us+vs)/512),loader+0x1FA);   //end lba
    
    // magic
    loader[0x1FE]=0x55; loader[0x1FF]=0xAA;

    // GPT Header
    memset(gpt,0,gs);
    memcpy(gpt,"EFI PART",8);                   //magic
    setint(1,gpt+10);                           //revision
    setint(92,gpt+12);                          //size
    setint(0xDEADCC32,gpt+16);                  //crc
    setint(1,gpt+24);                           //primarylba
    setint(((gs+es+us+vs+hs)/512)+3,gpt+32);    //backuplba
    setint(34,gpt+40);                          //firstusable
    setint(((gs+es+us+vs+hs)/512)+1,gpt+48);    //lastusable
    setint(uuid[0],gpt+56);                     //disk UUID
    setint(uuid[1],gpt+60);
    setint(uuid[2],gpt+64);
    setint(uuid[3],gpt+68);
    setint(2,gpt+72);                           //partitionlba
    setint(4,gpt+80);                           //numentries
    setint(128,gpt+84);                         //entrysize
    setint(0xDEADCC32,gpt+88);                  //entriescrc

    // GPT, EFI System Partition (mounted at /boot)
    setint(0x0C12A7328,gpt+512);                    //entrytype
    setint(0x011D2F81F,gpt+516);
    setint(0x0A0004BBA,gpt+520);
    setint(0x03BC93EC9,gpt+524);
    setint(uuid[0]+1,gpt+528);                      //partition UUID
    setint(uuid[1],gpt+532);
    setint(uuid[2],gpt+536);
    setint(uuid[3],gpt+540);
    setint((gs/512)+1,gpt+544);                     //startlba
    setint(((gs+es)/512),gpt+552);                  //endlba
    memcpy(gpt+568,L"EFI System Partition",42);     //name
    
    // GPT, OS/Z System Partition (mounted at /usr)
    memcpy(gpt+512+128,"OS/Z",4);                   //entrytype, magic
    setint(256,gpt+516+128);                        //version 1.0, no flags
    memcpy(gpt+522+128,"usr",3);                    //mount point
    setint(uuid[0]+2,gpt+528+128);                  //partition UUID
    setint(uuid[1],gpt+532+128);
    setint(uuid[2],gpt+536+128);
    setint(uuid[3],gpt+540+128);
    setint(((gs+es)/512)+1,gpt+544+128);            //startlba
    setint(((gs+es+us)/512),gpt+552+128);           //endlba
    setint(3,gpt+560+128);                          //system+noefi flag
    memcpy(gpt+568+128,L"OS/Z System",22);          //name

    // GPT, OS/Z Public Data Partition (mounted at /var)
    memcpy(gpt+512+256,"OS/Z",4);                   //entrytype, magic
    setint(256,gpt+516+256);                        //version 1.0, no flags
    memcpy(gpt+522+256,"var",3);                    //mount point
    setint(uuid[0]+3,gpt+528+256);                  //partition UUID
    setint(uuid[1],gpt+532+256);
    setint(uuid[2],gpt+536+256);
    setint(uuid[3],gpt+540+256);
    setint(((gs+es+us)/512)+1,gpt+544+256);         //startlba
    setint(((gs+es+us+vs)/512),gpt+552+256);        //endlba
    setint(3,gpt+560+256);                          //system+noefi flag
    memcpy(gpt+568+256,L"OS/Z Public Data",32);     //name

    // GPT, OS/Z Home Partition (mounted at /home)
    memcpy(gpt+512+384,"OS/Z",4);                   //entrytype, magic
    setint(256,gpt+516+384);                        //version 1.0, no flags
    memcpy(gpt+522+384,"home",4);                   //mount point
    setint(uuid[0]+4,gpt+528+384);                  //partition UUID
    setint(uuid[1],gpt+532+384);
    setint(uuid[2],gpt+536+384);
    setint(uuid[3],gpt+540+384);
    setint(((gs+es+us+vs)/512)+1,gpt+544+384);      //startlba
    setint(((gs+es+us+vs+hs)/512),gpt+552+384);     //endlba
    setint(2,gpt+560+384);                          //noefi flag
    memcpy(gpt+568+384,L"OS/Z Private Data",34);    //name

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
    fwrite(esp,es,1,f);
    fwrite(usr,us,1,f);
    fwrite(var,vs,1,f);
    fwrite(home,hs,1,f);
    //write GPT partition entries again
    fwrite(gpt+512,gs-512,1,f);
    //write GPT backup header
    setint(((gs+es+us+vs+hs)/512)+(gs/512),gpt+24); //primarylba
    setint(1,gpt+32);                           //backuplba
    setint(((gs+es+us+vs+hs)/512)+1,gpt+72);    //partitionlba
    i=getint(gpt+12);   //header size
    setint(0,gpt+16);   //must be zero when calculating
    setint(eficrc32_calc(gpt,i),gpt+16);
    fwrite(gpt,512,1,f);
    fclose(f);
    return 1;
}


//creates an fs image of a directory
int createimage(char *image,char *dir)
{
    fs=malloc(secsize);
    if(fs==NULL) exit(3);
    size=0;
    // superblock
    add_superblock();
    // add root directory
    ((FSZ_SuperBlock *)fs)->rootdirfid = add_inode("dir:",NULL);

    // recursively parse the directory and add everything in it
    // into the fs image
    add_dirs(dir,0,0);

    //modify the total number of sectors
    ((FSZ_SuperBlock *)fs)->numsec=size/secsize;
    ((FSZ_SuperBlock *)fs)->checksum=crc32_calc((char *)((FSZ_SuperBlock *)fs)->magic,180);

    //write out new image
    f=fopen(image,"wb");
    fwrite(fs,size,1,f);
    fclose(f);
    return 1;
}

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
    FSZ_DirEntHeader *hdr=(FSZ_DirEntHeader *)((char*)dir+1024);
    FSZ_DirEnt *ent;
    ent=(FSZ_DirEnt *)hdr; ent++;
    while(ent->fid!=0 && cnt<((secsize-1024)/128-1)) {
        FSZ_Inode *in = (FSZ_Inode *)((char*)data+ent->fid*secsize);
        printf("%c%c%c%c %6ld %6ld %s\n",
            in->filetype[0],in->filetype[1],in->filetype[2],in->filetype[3],
            in->sec, in->size, ent->name);
        ent++; cnt++;
    }
}

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

int main(int argc, char **argv)
{
    char *path=strdup(argv[0]);
    int i;

    checkcompilation();

    //get filenames
    for(i=strlen(path);i>0;i--) {if(path[i-1]=='/') break;}
    memcpy(path+i,"../bin/",8); i+=8; path[i]=0;
    diskname=malloc(i+16); sprintf(diskname,"%sdisk.dd",path);
    stage1=malloc(i+16); sprintf(stage1,"%s/../loader/mbr.bin",path);
    espfile=malloc(i+16); sprintf(espfile,"%sesp.part",path);
    usrfile=malloc(i+16); sprintf(usrfile,"%susr.part",path);
    varfile=malloc(i+16); sprintf(varfile,"%svar.part",path);
    homefile=malloc(i+16); sprintf(homefile,"%shome.part",path);

    //parse arguments
    if(argv[1]==NULL||!strcmp(argv[1],"help")) {
        printf("FS/Z mkfs utility\n"
            "./mkfs (imagetoread) cat (path)\n"
            "./mkfs (imagetoread) ls (path)\n"
            "./mkfs (imagetocreate) (createfromdir)\n"
            "./mkfs disk\n");
        exit(0);
    }
    if(argc>1 && argv[2]!=NULL){
        if(!strcmp(argv[2],"cat")) {
            cat(argc,argv);
        } else
        if(!strcmp(argv[2],"ls")) {
            ls(argc,argv);
        } else
        if(!strcmp(argv[2],"disk")) {
            createdisk();
        } else
        if(argc>2) {
            createimage(argv[2],argv[3]);
        } else {
            printf("mkfs: unknown command.\n");
        }
    }
    return 0;
}
