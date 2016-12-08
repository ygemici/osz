/*
 * bootboot.c
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
 * @brief Booting code for EFI
 * 
 */
#include <elf.h>
#include <efi.h>
#include <efilib.h>
#include <efiprot.h>
#define _BOOTBOOT_LOADER 1
#include "../bootboot.h"
#include "zlib_inflate/zlib.h"

#if PRINT_DEBUG
#define DBG(fmt, ...) do{Print(fmt,__VA_ARGS__); }while(0);
#else
#define DBG(fmt, ...)
#endif


extern EFI_STATUS zlib_inflate_blob(void *gunzip_buf, unsigned int sz,
		      const void *buf, unsigned int len);
extern EFI_GUID GraphicsOutputProtocol;
extern EFI_GUID LoadedImageProtocol;
struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;
struct EFI_FILE_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI *EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_OPEN_VOLUME)(
  IN struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL    *This,
  OUT struct EFI_FILE_PROTOCOL                 **Root
  );

typedef struct _EFI_SIMPLE_FILE_SYSTEM_PROTOCOL {
  UINT64                                      Revision;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_OPEN_VOLUME OpenVolume;
} EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;

typedef struct _EFI_FILE_PROTOCOL {
  UINT64                Revision;
  EFI_FILE_OPEN         Open;
  EFI_FILE_CLOSE        Close;
  EFI_FILE_DELETE       Delete;
  EFI_FILE_READ         Read;
  EFI_FILE_WRITE        Write;
  EFI_FILE_GET_POSITION GetPosition;
  EFI_FILE_SET_POSITION SetPosition;
  EFI_FILE_GET_INFO     GetInfo;
  EFI_FILE_SET_INFO     SetInfo;
  EFI_FILE_FLUSH        Flush;
} EFI_FILE_PROTOCOL;

#define PAGESIZE 4096

UINT8 *env_ptr;
UINTN env_len;
UINT8 *initrd_ptr;
UINTN initrd_len;
UINT8 *core_ptr;
UINTN core_len;
BOOTBOOT *bootboot;
UINT64 *paging;
UINT64 entrypoint;
EFI_SIMPLE_FILE_SYSTEM_PROTOCOL	*Volume;
EFI_FILE_HANDLE					RootDir;
EFI_FILE_PROTOCOL				*Root;
int reqwidth, reqheight;
char *kernelname="lib/sys/core";
unsigned char *kne;
#if PRINT_DEBUG
int dbg=0;
#endif

int atoi(unsigned char*c)
{
    int r=0;
    while(*c>='0'&&*c<='9') {
        r*=10; r+=*c-'0';
        c++;
    }
    return r;
}

CHAR16 *
a2u (char *str)
{
    static CHAR16 mem[4096];
    int i;

    for (i = 0; str[i]; ++i)
        mem[i] = (CHAR16) str[i];
    mem[i] = 0;
    return mem;
}

EFI_STATUS
report(EFI_STATUS Status,CHAR16 *Msg)
{
	Print(L"BOOTBOOT-PANIC: %s (EFI %r)\n",Msg,Status);
	return Status;
}

int oct2bin(unsigned char *str,int size)
{
	int s=0;
	unsigned char *c=str;
	while(size-->0){
		s*=8;
		s+=*c-'0';
		c++;
	}
	return s;
}

#include "fs.h"

EFI_STATUS
ParseEnvironment(unsigned char *cfg, int len)
{
    unsigned char *ptr=cfg-1;
	DBG(L" * Environment @%lx %d bytes\n",cfg,len);
    while(ptr<cfg+len && ptr[0]!=0) {
		ptr++;
        if(ptr[0]==' '||ptr[0]=='\t'||ptr[0]=='\r'||ptr[0]=='\n')
            continue;
        if(ptr[0]=='/'&&ptr[1]=='/') {
            while(ptr<cfg+len && ptr[0]!='\r' && ptr[0]!='\n' && ptr[0]!=0){
				ptr++;
			}
			ptr--;
            continue;
        }
        if(!CompareMem(ptr,(const CHAR8 *)"width=",6)){
            ptr+=6;
            reqwidth=atoi(ptr);
        }
        if(!CompareMem(ptr,(const CHAR8 *)"height=",7)){
            ptr+=7;
            reqheight=atoi(ptr);
        }
        if(!CompareMem(ptr,(const CHAR8 *)"kernel=",7)){
            ptr+=7;
            kernelname=(char*)ptr;
            while(ptr<cfg+len && ptr[0]!='\r' && ptr[0]!='\n' &&
				ptr[0]!=' ' && ptr[0]!='\t' && ptr[0]!=0)
					ptr++;
			kne=ptr;
			*ptr=0;
			ptr++;
        }
    }
    if(reqwidth<640) { reqwidth=640; }
    if(reqheight<400) { reqheight=400; }
	return EFI_SUCCESS;
}

EFI_STATUS
LoadFile(IN CHAR16 *FileName, OUT UINT8 **FileData, OUT UINTN *FileDataLength)
{
	EFI_STATUS          Status;
	EFI_FILE_HANDLE     FileHandle;
	EFI_FILE_INFO       *FileInfo;
	UINT64              ReadSize;
	UINTN               BufferSize;
	UINT8               *Buffer;
	
	if ((RootDir == NULL) || (FileName == NULL)) {
		return report(EFI_NOT_FOUND,L"Empty Root or FileName\n");
	}
	
	Status = uefi_call_wrapper(RootDir->Open, 5, RootDir, &FileHandle, FileName, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
	if (EFI_ERROR(Status)) {
		report(Status,L"Open error");
		Print(L"  File: %s\n",FileName);
		return Status;
	}
	FileInfo = LibFileInfo(FileHandle);
	if (FileInfo == NULL) {
		uefi_call_wrapper(FileHandle->Close, 1, FileHandle);
		report(EFI_NOT_FOUND,L"FileInfo error");
		Print(L"  File: %s\n",FileName);
		return EFI_NOT_FOUND;
	}
	ReadSize = FileInfo->FileSize;
	if (ReadSize > 16*1024*1024)
		ReadSize = 16*1024*1024;
	FreePool(FileInfo);
	
	BufferSize = (UINTN)((ReadSize+4095)/4096);
	Status = uefi_call_wrapper(BS->AllocatePages, 4, 0, 2, BufferSize, (EFI_PHYSICAL_ADDRESS*)&Buffer);
	if (Buffer == NULL) {
		uefi_call_wrapper(FileHandle->Close, 1, FileHandle);
		return report(EFI_OUT_OF_RESOURCES,L"AllocatePages");
	}
	Status = uefi_call_wrapper(FileHandle->Read, 3, FileHandle, &ReadSize, Buffer);
	uefi_call_wrapper(FileHandle->Close, 1, FileHandle);
	if (EFI_ERROR(Status)) {
		uefi_call_wrapper(BS->FreePages, 2, (EFI_PHYSICAL_ADDRESS)(Buffer), BufferSize);
		report(Status,L"Read error");
		Print(L"  File: %s\n",FileName);
		return Status;
	}
	
	*FileData = Buffer;
	*FileDataLength = ReadSize;
	return EFI_SUCCESS;
}

EFI_STATUS
SetMode(int mode,EFI_GRAPHICS_OUTPUT_PROTOCOL *gop)
{
	EFI_STATUS rc;
	bootboot->fb_ptr=0;
	bootboot->fb_size=0;
#if PRINT_DEBUG
	rc = mode ? EFI_SUCCESS : EFI_SUCCESS;
#else
	rc = uefi_call_wrapper(gop->SetMode, 2, gop, mode);
#endif
	if(!EFI_ERROR(rc)){
		bootboot->fb_ptr=(void*)gop->Mode->FrameBufferBase;
		bootboot->fb_size=gop->Mode->FrameBufferSize;
		bootboot->fb_scanline=4*gop->Mode->Info->PixelsPerScanLine;
		bootboot->fb_width=gop->Mode->Info->HorizontalResolution;
		bootboot->fb_height=gop->Mode->Info->VerticalResolution;

		bootboot->fb_type=
			gop->Mode->Info->PixelFormat==PixelBlueGreenRedReserved8BitPerColor ||
			(gop->Mode->Info->PixelFormat==PixelBitMask && gop->Mode->Info->PixelInformation.BlueMask==0)? FB_ARGB : (
				gop->Mode->Info->PixelFormat==PixelRedGreenBlueReserved8BitPerColor ||
				(gop->Mode->Info->PixelFormat==PixelBitMask && gop->Mode->Info->PixelInformation.RedMask==0)? FB_ABGR : (
					gop->Mode->Info->PixelInformation.BlueMask==8? FB_RGBA : FB_BGRA
			));
	}
	DBG(L" * Screen %d x %d, scanline %d, fb @%lx %d bytes\n",bootboot->fb_width,bootboot->fb_height,bootboot->fb_scanline,bootboot->fb_ptr,bootboot->fb_size);
	return rc;
}

EFI_STATUS
SetScreenRes(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop)
{
	int i, firstgood=-1, imax=gop->Mode->MaxMode;
	EFI_STATUS rc;
	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
	UINTN SizeOfInfo;

	for (i = 0; i < imax; i++) {
		rc = uefi_call_wrapper(gop->QueryMode, 4, gop, i, &SizeOfInfo,
					&info);
		if (EFI_ERROR(rc) && rc == EFI_NOT_STARTED) {
			rc = uefi_call_wrapper(gop->SetMode, 2, gop,
				gop->Mode->Mode);
			rc = uefi_call_wrapper(gop->QueryMode, 4, gop, i,
				&SizeOfInfo, &info);
		}

		if (EFI_ERROR(rc)) {
			continue;
		}
		if(info->PixelFormat!=PixelRedGreenBlueReserved8BitPerColor &&
		   info->PixelFormat!=PixelBlueGreenRedReserved8BitPerColor &&
		  (info->PixelFormat!=PixelBitMask||info->PixelInformation.ReservedMask!=0))
			continue;
		if(firstgood==-1)
			firstgood=gop->Mode->Mode;
		if(info->HorizontalResolution==(unsigned int)reqwidth && info->VerticalResolution==(unsigned int)reqheight) {
			return SetMode(i, gop);
		}
	}
	if(firstgood!=-1) {
		return SetMode(firstgood, gop);
	}

	return EFI_SUCCESS;
}

EFI_STATUS
LoadCore(UINT8 *initrd_ptr)
{
	int i=0,len=0;
	UINT8 *ptr;
	EFI_STATUS rc;
	//check if initrd is gzipped
	if(initrd_ptr[0]==0x1f&&initrd_ptr[1]==0x8b){
		unsigned char *addr;
		CopyMem(&len,initrd_ptr+initrd_len-4,4);
		uefi_call_wrapper(BS->AllocatePages, 4, 0, 2, (len+4095)/4096, (EFI_PHYSICAL_ADDRESS*)&addr);
		if(addr==0)
			return report(EFI_OUT_OF_RESOURCES,L"gzip inflate\n");
		DBG(L" * Gzip detected, uncompressed initrd @%lx %d bytes\n",addr,len);
		return report(EFI_LOAD_ERROR,L"compressed initrd not supported yet");
		// this ain't workin'
		rc = zlib_inflate_blob(addr, len, initrd_ptr, initrd_len);
		if(EFI_ERROR(rc))
			return report(rc,L"inflating initrd");
		// swap initrd_ptr with the uncompressed buffer
		uefi_call_wrapper(BS->FreePages, 2, (EFI_PHYSICAL_ADDRESS)initrd_ptr, (initrd_len+4095)/4096);
		initrd_ptr=addr;
		initrd_len=len;
	}

	core_ptr=ptr=NULL;
	core_len=0;
	while(ptr==NULL && fsdrivers[i]!=NULL) {
		ptr=(UINT8*)(*fsdrivers[i++])((unsigned char*)initrd_ptr,kernelname);
	}
	// if every driver failed, try brute force scan
	if(ptr==NULL) {
		i=initrd_len;
		ptr=initrd_ptr;
		while(i-->0) {
			Elf64_Ehdr *ehdr=(Elf64_Ehdr *)(ptr);
			if(!CompareMem(ehdr->e_ident,ELFMAG,SELFMAG)&&
				ehdr->e_ident[EI_CLASS]==ELFCLASS64&&
				ehdr->e_ident[EI_DATA]==ELFDATA2LSB&&
				ehdr->e_type==ET_EXEC&&ehdr->e_phnum>0){
					break;
				}
			ptr++;
		}
		ptr=NULL;
	}

	DBG(L" * Parsing ELF64 @%lx\n",ptr);
	if(ptr!=NULL) {
		// Parse ELF64
		Elf64_Ehdr *ehdr=(Elf64_Ehdr *)(ptr);
		if(CompareMem(ehdr->e_ident,ELFMAG,SELFMAG)||
			ehdr->e_ident[EI_CLASS]!=ELFCLASS64||
			ehdr->e_ident[EI_DATA]!=ELFDATA2LSB||
			ehdr->e_type!=ET_EXEC||ehdr->e_phnum==0){
				return report(EFI_LOAD_ERROR,L"Kernel is not an executable ELF64 for x86_64");
			}
		// Check program header
		Elf64_Phdr *phdr=(Elf64_Phdr *)((UINT8 *)ehdr+ehdr->e_phoff);
		for(i=0;i<ehdr->e_phnum;i++){
			if(phdr->p_type==PT_LOAD && phdr->p_vaddr>>48==0xffff) {
				core_len = ((phdr->p_filesz+4096-1)/4096)*4096;
				// Allocate memory (2M)
				uefi_call_wrapper(BS->AllocatePages, 4, 0, 2, 2*1024*1024/4096, (EFI_PHYSICAL_ADDRESS*)&core_ptr);
				if (core_ptr == NULL) {
					return report(EFI_OUT_OF_RESOURCES,L"AllocatePages");
				}
				ZeroMem(core_ptr,2*1024*1024);
				entrypoint=ehdr->e_entry;
				DBG(L" * Entry point @%lx, offset @%lx, text @%lx %d bytes\n",entrypoint, (UINT8 *)ehdr+phdr->p_offset, core_ptr, core_len);
				CopyMem(core_ptr,(UINT8 *)ehdr+phdr->p_offset,core_len);
				return EFI_SUCCESS;
			}
			phdr=(Elf64_Phdr *)((UINT8 *)phdr+ehdr->e_phentsize);
		}
		return report(EFI_LOAD_ERROR,L"Kernel does not have a valid text segment.");
	}
	return report(EFI_LOAD_ERROR,L"Kernel not found in initrd");
}

/**
 * Main EFI application entry point
 */
EFI_STATUS
efi_main (EFI_HANDLE image, EFI_SYSTEM_TABLE *systab)
{
	EFI_LOADED_IMAGE *loaded_image = NULL;
	EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
	EFI_STATUS status=EFI_SUCCESS;

	CHAR16 *initrdfile;
	CHAR16 *configfile;
	CHAR16 **argv;
	INTN argc;
	UINTN i;
	CHAR16 *help=L"SYNOPSIS\n  BOOTBOOT.EFI [-?|-h|/?|/h] [INITRDFILE [ENVIRONMENTFILE]]\n\nDESCRIPTION\n  Bootstraps the OS/Z (or any other compatible) operating system via\n  the BOOTBOOT Protocol. If not given, the loader defaults to\n   FS0:\\BOOTBOOT\\INITRD as ramdisk image and\n   FS0:\\BOOTBOOT\\CONFIG for boot environment.\n  This is a loader, it is not supposed to return control to the shell.\n\n";

	// Initialize UEFI Library
	InitializeLib(image, systab);
	status = uefi_call_wrapper(systab->BootServices->HandleProtocol,
				3,
				image, 
				&LoadedImageProtocol, 
				(void **) &loaded_image);
	if (EFI_ERROR(status)) {
		return report(status, L"HandleProtocol");
	}

	// Parse command line arguments
	// BOOTX86.EFI [-?|-h|/?|/h] [initrd [config]]
	argc = GetShellArgcArgv(image, &argv);
	if(argc>1) {
		if((argv[1][0]=='-'||argv[1][0]=='/')&&(argv[1][1]=='?'||argv[1][1]=='h')){
			Print(L"OS/Z LOADER (build %s)\n\n%s",a2u(__DATE__),help);
			return EFI_SUCCESS;
		}
		initrdfile=argv[1];
	} else {
		initrdfile=L"\\BOOTBOOT\\INITRD";
	}
	if(argc>2) {
		configfile=argv[2];
	} else {
		configfile=L"\\BOOTBOOT\\CONFIG";
	}
	Print(L"OS/Z booting...\n");

	// Initialize FS with the DeviceHandler from loaded image protocol
	RootDir = LibOpenRoot(loaded_image->DeviceHandle);

	// load ramdisk
	status=LoadFile(initrdfile,&initrd_ptr, &initrd_len);
	if(status==EFI_SUCCESS){
		DBG(L" * Initrd loaded @%lx %d bytes\n",initrd_ptr,initrd_len);
		// get memory for bootboot structure
		uefi_call_wrapper(BS->AllocatePages, 4, 0, 2, 1, (EFI_PHYSICAL_ADDRESS*)&bootboot);
		if (bootboot == NULL) {
			return report(EFI_OUT_OF_RESOURCES,L"AllocatePages\n");
		}
		kne=NULL;
		// if there's an evironment file, load it
		if(LoadFile(configfile,&env_ptr,&env_len)==EFI_SUCCESS)
			ParseEnvironment(env_ptr,env_len);
		else {
			env_len=0;
			uefi_call_wrapper(BS->AllocatePages, 4, 0, 2, 1, (EFI_PHYSICAL_ADDRESS*)&env_ptr);
			if (env_ptr == NULL) {
				return report(EFI_OUT_OF_RESOURCES,L"AllocatePages\n");
			}
			ZeroMem((void*)env_ptr,4096);
			CopyMem((void*)env_ptr,"// N/A",8);
		}

		// Ok, it's time to collect information on system
		ZeroMem((void*)bootboot,4096);
		CopyMem(bootboot->magic,BOOTPARAMS_MAGIC,4);
		__asm__ __volatile__ (
			"mov $1, %%eax;"
			"cpuid;"
			"shrl $24, %%ebx;"
			"mov %%ebx,%0"
			: : "b"(bootboot->bspid) : "memory" );
		bootboot->protocol=PROTOCOL_STATIC;
		bootboot->loader_type=LOADER_UEFI;
		bootboot->size=128;
		bootboot->pagesize=PAGESIZE;
		CopyMem((void *)&(bootboot->initrd_ptr),&initrd_ptr,8);
		bootboot->initrd_size=((initrd_len+4095)/4096)*4096;
		UINT64 p=0xFFFFFFFFFFE00000+(uint64_t)(&(bootboot->mmap))-(uint64_t)(&(bootboot->magic));
		CopyMem(&(bootboot->mmap_ptr),&p,8);
		CopyMem((void *)&(bootboot->efi_ptr),&systab,8);

		// System tables and structures
		LibGetSystemConfigurationTable(&AcpiTableGuid,(void *)&(bootboot->acpi_ptr));
		LibGetSystemConfigurationTable(&SMBIOSTableGuid,(void *)&(bootboot->smbi_ptr));
		LibGetSystemConfigurationTable(&MpsTableGuid,(void *)&(bootboot->mp_ptr));

		// Date and time
		EFI_TIME t;
		uint8_t bcd[8];
		uefi_call_wrapper(ST->RuntimeServices->GetTime, 2, &t, NULL);
		bcd[0]=DecimaltoBCD(t.Year/100);
		bcd[1]=DecimaltoBCD(t.Year%100);
		bcd[2]=DecimaltoBCD(t.Month);
		bcd[3]=DecimaltoBCD(t.Day);
		bcd[4]=DecimaltoBCD(t.Hour);
		bcd[5]=DecimaltoBCD(t.Minute);
		bcd[6]=DecimaltoBCD(t.Second);
		bcd[7]=DecimaltoBCD(t.Daylight);
		CopyMem((void *)&bootboot->datetime, &bcd, 8);
		CopyMem((void *)&bootboot->timezone, &t.TimeZone, 2);
		if(bootboot->timezone<-1440||bootboot->timezone>1440)	// TZ in mins
			bootboot->timezone=0;
		DBG(L" * System time %d-%02d-%02d %02d:%02d:%02d GMT%s%d:%02d %s\n",
			t.Year,t.Month,t.Day,t.Hour,t.Minute,t.Second,
			bootboot->timezone>=0?L"+":L"",bootboot->timezone/60,bootboot->timezone%60,
			t.Daylight?L"summertime":L"");
		// get lib/sys/core and parse
		status=LoadCore(initrd_ptr);
		if (EFI_ERROR(status))
			return status;
		if(kne!=NULL)
			*kne=' ';

		//WaitForSingleEvent(ST->ConIn->WaitForKey, 0);

		//GOP
		status = LibLocateProtocol(&GraphicsOutputProtocol, (void **)&gop);
		if (EFI_ERROR(status))
			return report(status,L"Locate GOP\n");
		status = SetScreenRes(gop);
		// TODO: if GOP fails, fallback to UGA
		if (EFI_ERROR(status))
			return report(EFI_DEVICE_ERROR,L"Initialize screen\n");

		// create page tables
		uefi_call_wrapper(BS->AllocatePages, 4, 0, 2, 23, (EFI_PHYSICAL_ADDRESS*)&paging);
		if (paging == NULL) {
			return report(EFI_OUT_OF_RESOURCES,L"AllocatePages\n");
		}
		ZeroMem((void*)paging,23*4096);
		DBG(L" * Pagetables PML4 @%lx\n",paging);
		//PML4
		paging[0]=(UINT64)((UINT8 *)paging+4*4096)+1;	// pointer to 2M PDPE (16G RAM identity mapped)
		paging[511]=(UINT64)((UINT8 *)paging+4096)+1;	// pointer to 4k PDPE (core mapped at -2M)
		//4k PDPE
		paging[512+511]=(UINT64)((UINT8 *)paging+2*4096+1);
		//4k PDE
		for(i=0;i<255;i++)
			paging[2*512+256+i]=(UINT64)(((UINT8 *)(bootboot->fb_ptr)+(i<<21))+0x81);	//map framebuffer
		paging[2*512+511]=(UINT64)((UINT8 *)paging+3*4096+1);
		//4k PT
		paging[3*512+0]=(UINT64)(bootboot)+1;
		paging[3*512+1]=(UINT64)(env_ptr)+1;
		for(i=0;i<(core_len/4096);i++)
			paging[3*512+2+i]=(UINT64)((UINT8 *)core_ptr+i*4096+1);
		paging[3*512+511]=(UINT64)((UINT8 *)paging+22*4096+1);
		//identity mapping
		//2M PDPE
		for(i=0;i<16;i++)
			paging[4*512+i]=(UINT64)((UINT8 *)paging+(7+i)*4096+1);
		//first 2M mapped per page
		paging[7*512]=(UINT64)((UINT8 *)paging+5*4096+1);
		for(i=0;i<512;i++)
			paging[5*512+i]=(UINT64)(i*4096+1);
		//2M PDE
		for(i=1;i<512*16;i++)
			paging[7*512+i]=(UINT64)((i<<21)+0x81);

		// Get memory map
		int cnt=10;
		UINTN                 memory_map_size;
		EFI_MEMORY_DESCRIPTOR *memory_map, *mement;
		UINTN                 map_key;
		UINTN                 desc_size;
		UINT32                desc_version;
		MMapEnt               *mmapent;
get_memory_map:
		mmapent=(MMapEnt *)&(bootboot->mmap);
		memory_map = LibMemoryMap(&memory_map_size, &map_key, &desc_size, &desc_version);
		if (memory_map == NULL) {
			return report(EFI_OUT_OF_RESOURCES,L"LibMemoryMap\n");
		}
		for(mement=memory_map;mement<memory_map+memory_map_size;mement=NextMemoryDescriptor(mement,desc_size)) {
			mmapent->ptr=mement->PhysicalStart;
			mmapent->size=(mement->NumberOfPages*4096)+
				((mement->Type>0&&mement->Type<5)||mement->Type==7?MMAP_FREE:
				(mement->Type==8?MMAP_BAD:
				(mement->Type==9?MMAP_ACPIFREE:
				(mement->Type==10?MMAP_ACPINVS:
				(mement->Type==11||mement->Type==12?MMAP_MMIO:
				MMAP_RESERVED)))));
			bootboot->size+=16;
			mmapent++;
			if(bootboot->size>=4096) break;
		}
		// --- NO PRINT AFTER THIS POINT ---
		
		//inform firmware that we're about to leave it's realm
		status = uefi_call_wrapper(BS->ExitBootServices, 2, image, map_key);
		if(EFI_ERROR(status)){
			cnt--;
			if(cnt>0) goto get_memory_map;
			return report(status,L"ExitBootServices");
		}

		//set up paging
		__asm__ __volatile__ (
			"mov %0,%%rax;"
			"mov %%rax,%%cr3"
			: : "b"(paging) : "memory" );

		//call _start() in lib/sys/core
		__asm__ __volatile__ (
			"xor %%rsp, %%rsp;"
			"push %0;"
			"mov $0x544f4f42544f4f42,%%rax;"	// boot magic 'BOOTBOOT'
			"mov $0xffffffffffe00000,%%rbx;"	// bootboot virtual address
			"mov $0xffffffffffe01000,%%rcx;"	// environment virtual address
			"mov $0xffffffffe0000000,%%rdx;"	// framebuffer virtual address
			"ret"
			: : "r"(entrypoint): "memory" );
	}

	return status;
}

