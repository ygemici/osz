/*
 * fs.h
 * 
 * Copyright 2016 BOOTBOOT bztsrc@github
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 * This file is part of the BOOTBOOT Protocol package.
 * @brief Filesystem drivers for initial ramdisk.
 * 
 */

#include "../../etc/include/fsZ.h"

/**
 * FS/Z (BOOTBOOT's and OS/Z's native file system)
 */
unsigned char *fsz_initrd(unsigned char *initrd_p, char *kernel)
{
	FSZ_SuperBlock *sb = (FSZ_SuperBlock *)initrd_p;
	FSZ_DirEnt *ent;
	FSZ_Inode *in=(FSZ_Inode *)(initrd_p+sb->rootdirfid*FSZ_SECSIZE);
	if(initrd_p==NULL || CompareMem(sb->magic,FSZ_MAGIC,4) || kernel==NULL){
		return NULL;
	}
	// Make sure only files in lib/ will be loaded
	CopyMem(kernel,"lib/",4);
	DBG(L" * fs/Z rootdir inode %d @%lx %s\n",sb->rootdirfid,in,a2u(kernel));
	// Get the inode of lib/core
	int i;
	char *s,*e;
	s=e=kernel;
	i=0;
again:
	while(*e!='/'&&*e!=0){e++;}
	if(*e=='/'){e++;}
	if(!CompareMem(in->magic,FSZ_IN_MAGIC,4)){
		//is it inlined?
		if(!CompareMem(in->inlinedata,FSZ_DIR_MAGIC,4)){
			ent=(FSZ_DirEnt *)(in->inlinedata);
		} else if(!CompareMem(initrd_p+in->sec*FSZ_SECSIZE,FSZ_DIR_MAGIC,4)){
			// go, get the sector pointed
			ent=(FSZ_DirEnt *)(initrd_p+in->sec*FSZ_SECSIZE);
		} else {
			return NULL;
		}
		//skip header
		FSZ_DirEntHeader *hdr=(FSZ_DirEntHeader *)ent; ent++;
		//iterate on directory entries
		int j=hdr->numentries;
		while(j-->0){
			if(!CompareMem(ent->name,s,e-s)) {
				if(*e==0) {
					i=ent->fid;
					break;
				} else {
					s=e;
					in=(FSZ_Inode *)(initrd_p+ent->fid*FSZ_SECSIZE);
					goto again;
				}
			}
			ent++;
		}
	} else {
		i=0;
	}
	DBG(L" * Kernel inode %d @%lx\n",i,i?initrd_p+i*FSZ_SECSIZE:0);
	if(i!=0) {
		// fid -> inode ptr -> data ptr
		FSZ_Inode *in=(FSZ_Inode *)(initrd_p+i*FSZ_SECSIZE);
		if(!CompareMem(in->magic,FSZ_IN_MAGIC,4)){
			Elf64_Ehdr *ehdr=(Elf64_Ehdr *)(initrd_p + in->sec * FSZ_SECSIZE);
			if(!CompareMem(ehdr->e_ident,ELFMAG,SELFMAG))
				return (initrd_p + in->sec * FSZ_SECSIZE);
			else
				return (initrd_p + (unsigned int)(((FSZ_SectorList *)(initrd_p+in->sec*FSZ_SECSIZE))->fid) * FSZ_SECSIZE);
		}
	}
	return NULL;
}

/**
 * cpio hpodc archive
 */
unsigned char *cpio_initrd(unsigned char *initrd_p, char *kernel)
{
	unsigned char *ptr=initrd_p;
	int k;
	if(initrd_p==NULL || kernel==NULL || CompareMem(initrd_p,"070707",6))
		return NULL;
	DBG(L" * cpio %s\n",a2u(kernel));
	k=strlena((unsigned char*)kernel);
	while(!CompareMem(ptr,"070707",6)){
		int ns=oct2bin(ptr+8*6+11,6);
		int fs=oct2bin(ptr+8*6+11+6,11);
		if(!CompareMem(ptr+9*6+2*11,kernel,k+1)){
			return (unsigned char*)(ptr+9*6+2*11+ns);
		}
		ptr+=(76+ns+fs);
	}
	return NULL;
}

/**
 * ustar tarball archive
 */
unsigned char *tar_initrd(unsigned char *initrd_p, char *kernel)
{
	unsigned char *ptr=initrd_p;
	int k;
	if(initrd_p==NULL || kernel==NULL || CompareMem(initrd_p+257,"ustar",5))
		return NULL;
	DBG(L" * tar %s\n",a2u(kernel));
	k=strlena((unsigned char*)kernel);
	while(!CompareMem(ptr+257,"ustar",5)){
		int fs=oct2bin(ptr+0x7c,11);
		if(!CompareMem(ptr,kernel,k+1)){
			return (unsigned char*)(ptr+512);
		}
		ptr+=(((fs+511)/512)+1)*512;
	}
	return NULL;
}

/**
 * FAT 12
 */
unsigned char *fat12_initrd(unsigned char *initrd_p, char *kernel)
{
	if(initrd_p==NULL || kernel==NULL || CompareMem(initrd_p+0x36,"FAT12",5)) {
		return NULL;
	}
	DBG(L" * FAT12 %s unimplemented\n",a2u(kernel));
	return NULL;
}

/**
 * FAT 16
 */
unsigned char *fat16_initrd(unsigned char *initrd_p, char *kernel)
{
	if(initrd_p==NULL || kernel==NULL || CompareMem(initrd_p+0x36,"FAT16",5)){
		return NULL;
	}
	DBG(L" * FAT16 %s unimplemented\n",a2u(kernel));
	return NULL;
}

/**
 * Static file system drivers registry
 */
unsigned char* (*fsdrivers[]) (unsigned char *, char *) = {
	fsz_initrd,
	cpio_initrd,
	tar_initrd,
	fat12_initrd,
	fat16_initrd,
	NULL
};
