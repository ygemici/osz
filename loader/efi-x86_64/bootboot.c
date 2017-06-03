/*
 * loader/efi-x86_64/bootboot.c
 *
 * Copyright 2016 Public Domain BOOTBOOT bztsrc@github
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
#include "tinf.h"
// just comment out this include if you don't want FS/Z support
#include "../../etc/include/fsZ.h"

#define DBG(fmt, ...) do{Print(fmt,__VA_ARGS__); }while(0);
//#define DBG(fmt, ...)

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

/* Intel EFI headers has simple file protocol, but not GNU EFI */
#ifndef EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION
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
#endif

typedef struct {
    UINT8 magic[8];
    UINT8 chksum;
    CHAR8 oemid[6];
    UINT8 revision;
    UINT32 rsdt;
    UINT32 length;
    UINT64 xsdt;
    UINT32 echksum;
} __attribute__((packed)) ACPI_RSDPTR;

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
EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Volume;
EFI_FILE_HANDLE                 RootDir;
EFI_FILE_PROTOCOL               *Root;
unsigned char *kne;

// default environment variables
int reqwidth = 800, reqheight = 600;
char *kernelname="lib/sys/core";

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
    static CHAR16 mem[PAGESIZE];
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

/**
 * Parse FS0:\BOOTBOOT\CONFIG
 */
EFI_STATUS
ParseEnvironment(unsigned char *cfg, int len, INTN argc, CHAR16 **argv)
{
    unsigned char *ptr=cfg-1;
    int i;
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
        if(argc>2 && !CompareMem(ptr,(const CHAR8 *)"\n\n\n",3)){
            ptr++;
            for(i=3;i<argc;i++) {
                CopyMem(ptr,argv[i],StrLen(argv[i]));
                ptr += StrLen(argv[i]);
                *ptr = '\n';
                ptr++;
            }
        }
    }
    return EFI_SUCCESS;
}

/**
 * Get a linear frame buffer
 */
EFI_STATUS
GetLFB()
{
    EFI_STATUS status;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
    EFI_GUID gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
    UINTN i, imax, SizeOfInfo, nativeMode, selectedMode=9999, sw=0, sh=0;

    //GOP
    status = uefi_call_wrapper(BS->LocateProtocol, 3, &gopGuid, NULL, (void**)&gop);
    if(EFI_ERROR(status))
        return status;

    // minimum resolution
    if(reqwidth < 640)  reqwidth = 640;
    if(reqheight < 480) reqheight = 480;

    // get current video mode
    status = uefi_call_wrapper(gop->QueryMode, 4, gop, gop->Mode==NULL?0:gop->Mode->Mode, &SizeOfInfo, &info);
    if (status == EFI_NOT_STARTED)
        status = uefi_call_wrapper(gop->SetMode, 2, gop, 0);
    if(EFI_ERROR(status))
        return status;
    nativeMode = gop->Mode->Mode;
    imax=gop->Mode->MaxMode;
    for (i = 0; i < imax; i++) {
        status = uefi_call_wrapper(gop->QueryMode, 4, gop, i, &SizeOfInfo, &info);
        // failsafe
        if (EFI_ERROR(status) || (
           info->PixelFormat != PixelRedGreenBlueReserved8BitPerColor &&
           info->PixelFormat != PixelBlueGreenRedReserved8BitPerColor &&
          (info->PixelFormat != PixelBitMask || info->PixelInformation.ReservedMask!=0)))
            continue;
        DBG(L"    %c%d %d x %d\n", i==nativeMode?'>':' ', i, info->HorizontalResolution, info->VerticalResolution);
        // get the mode for the closest resolution
        if( info->HorizontalResolution >= (unsigned int)reqwidth && 
            info->VerticalResolution >= (unsigned int)reqheight &&
            (selectedMode==9999||(info->HorizontalResolution<sw && info->VerticalResolution < sh))) {
                selectedMode = i;
                sw = info->HorizontalResolution;
                sh = info->VerticalResolution;
        }
    }
    // if we have found a new, better mode
    if(selectedMode != 9999 && selectedMode != nativeMode) {
        status = uefi_call_wrapper(gop->SetMode, 2, gop, selectedMode);
        if(!EFI_ERROR(status))
            nativeMode = selectedMode;
    }
    // get framebuffer properties
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
    DBG(L" * Screen %d x %d, scanline %d, fb @%lx %d bytes, mode %d\n",bootboot->fb_width,bootboot->fb_height,bootboot->fb_scanline,bootboot->fb_ptr,bootboot->fb_size, gop->Mode->Mode);
    return EFI_SUCCESS;
}

/**
 * Load a file from FS0 into memory
 */
EFI_STATUS
LoadFile(IN CHAR16 *FileName, OUT UINT8 **FileData, OUT UINTN *FileDataLength)
{
    EFI_STATUS          status;
    EFI_FILE_HANDLE     FileHandle;
    EFI_FILE_INFO       *FileInfo;
    UINT64              ReadSize;
    UINTN               BufferSize;
    UINT8               *Buffer;

    if ((RootDir == NULL) || (FileName == NULL)) {
        return report(EFI_NOT_FOUND,L"Empty Root or FileName\n");
    }

    status = uefi_call_wrapper(RootDir->Open, 5, RootDir, &FileHandle, FileName, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
    if (EFI_ERROR(status)) {
        report(status,L"Open error");
        Print(L"  File: %s\n",FileName);
        return status;
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

    BufferSize = (UINTN)((ReadSize+PAGESIZE-1)/PAGESIZE);
    status = uefi_call_wrapper(BS->AllocatePages, 4, 0, 2, BufferSize, (EFI_PHYSICAL_ADDRESS*)&Buffer);
    if (Buffer == NULL) {
        uefi_call_wrapper(FileHandle->Close, 1, FileHandle);
        return report(EFI_OUT_OF_RESOURCES,L"AllocatePages");
    }
    status = uefi_call_wrapper(FileHandle->Read, 3, FileHandle, &ReadSize, Buffer);
    uefi_call_wrapper(FileHandle->Close, 1, FileHandle);
    if (EFI_ERROR(status)) {
        uefi_call_wrapper(BS->FreePages, 2, (EFI_PHYSICAL_ADDRESS)(Buffer), BufferSize);
        report(status,L"Read error");
        Print(L"  File: %s\n",FileName);
        return status;
    }

    *FileData = Buffer;
    *FileDataLength = ReadSize;
    return EFI_SUCCESS;
}

/**
 * Locate and load the elf kernel in initrd
 */
EFI_STATUS
LoadCore(UINT8 *initrd_ptr)
{
    int i=0;
    UINT8 *ptr;

    core_ptr=ptr=NULL;
    core_len=0;
    while(ptr==NULL && fsdrivers[i]!=NULL) {
        ptr=(UINT8*)(*fsdrivers[i++])((unsigned char*)initrd_ptr,kernelname);
    }
    // if every driver failed, try brute force, scan for the first elf
    if(ptr==NULL) {
        i=initrd_len;
        ptr=initrd_ptr;
        while(i-->0) {
            Elf64_Ehdr *ehdr=(Elf64_Ehdr *)(ptr);
            if((!CompareMem(ehdr->e_ident,ELFMAG,SELFMAG)||!CompareMem(ehdr->e_ident,"OS/Z",4))&&
                ehdr->e_ident[EI_CLASS]==ELFCLASS64&&
                ehdr->e_ident[EI_DATA]==ELFDATA2LSB&&
                ehdr->e_phnum>0){
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
        if((CompareMem(ehdr->e_ident,ELFMAG,SELFMAG)&&CompareMem(ehdr->e_ident,"OS/Z",4))||
            ehdr->e_ident[EI_CLASS]!=ELFCLASS64||
            ehdr->e_ident[EI_DATA]!=ELFDATA2LSB||
            ehdr->e_phnum==0){
                return report(EFI_LOAD_ERROR,L"Kernel is not an executable ELF64 for x86_64");
            }
        // Check program header
        Elf64_Phdr *phdr=(Elf64_Phdr *)((UINT8 *)ehdr+ehdr->e_phoff);
        for(i=0;i<ehdr->e_phnum;i++){
            if(phdr->p_type==PT_LOAD && phdr->p_vaddr>>48==0xffff && phdr->p_offset==0) {
                core_len = ((phdr->p_filesz+PAGESIZE-1)/PAGESIZE)*PAGESIZE;
                core_ptr = (UINT8 *)ehdr;
                entrypoint=ehdr->e_entry;
                DBG(L" * Entry point @%lx, text @%lx %d bytes @%lx\n",entrypoint, 
                    core_ptr, core_len, (entrypoint/PAGESIZE)*PAGESIZE+core_len);
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
    EFI_GUID lipGuid = LOADED_IMAGE_PROTOCOL;
    EFI_STATUS status=EFI_SUCCESS;
    EFI_MEMORY_DESCRIPTOR *memory_map = NULL, *mement;
    UINTN i, memory_map_size=0, map_key=0, desc_size=0;
    UINT32 desc_version=0;
    MMapEnt *mmapent, *last=NULL;
    CHAR16 **argv, *initrdfile, *configfile, *help=
        L"SYNOPSIS\n  BOOTBOOT.EFI [ -h | -? | /h | /? ] [ INITRDFILE [ ENVIRONMENTFILE [...] ] ]\n\nDESCRIPTION\n  Bootstraps an operating system via the BOOTBOOT Protocol.\n  If arguments not given, defaults to\n    FS0:\\BOOTBOOT\\INITRD   as ramdisk image and\n    FS0:\\BOOTBOOT\\CONFIG   for boot environment.\n  Additional \"key=value\" arguments will be appended to environment.\n  As this is a loader, it is not supposed to return control to the shell.\n\n";
    INTN argc;
    CHAR8 *ptr;

    // Initialize UEFI Library
    InitializeLib(image, systab);
    BS = systab->BootServices;

    // Parse command line arguments
    // BOOTX86.EFI [-?|-h|/?|/h] [initrd [config]]
    argc = GetShellArgcArgv(image, &argv);
    if(argc>1) {
        if((argv[1][0]=='-'||argv[1][0]=='/')&&(argv[1][1]=='?'||argv[1][1]=='h')){
            Print(L"BOOTBOOT LOADER (build %s)\n\n%s",a2u(__DATE__),help);
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
    status = uefi_call_wrapper(systab->BootServices->HandleProtocol,
                3,
                image,
                &lipGuid,
                (void **) &loaded_image);
    if (EFI_ERROR(status)) {
        return report(status, L"HandleProtocol LoadedImageProtocol");
    }

    Print(L"Booting OS...\n");

    // get memory for bootboot structure
    uefi_call_wrapper(BS->AllocatePages, 4, 0, 2, 1, (EFI_PHYSICAL_ADDRESS*)&bootboot);
    if (bootboot == NULL)
        return report(EFI_OUT_OF_RESOURCES,L"AllocatePages");
    ZeroMem((void*)bootboot,PAGESIZE);
    CopyMem(bootboot->magic,BOOTPARAMS_MAGIC,4);

    // Initialize FS with the DeviceHandler from loaded image protocol
    RootDir = LibOpenRoot(loaded_image->DeviceHandle);

    // load ramdisk
    status=LoadFile(initrdfile,&initrd_ptr, &initrd_len);
    if(status==EFI_SUCCESS){
        //check if initrd is gzipped
        if(initrd_ptr[0]==0x1f && initrd_ptr[1]==0x8b){
            unsigned char *addr,f;
            int len=0, r;
            TINF_DATA d;
            DBG(L" * Gzip compressed initrd @%lx %d bytes\n",initrd_ptr,initrd_len);
            // skip gzip header
            addr=initrd_ptr+2;
            if(*addr++!=8) goto gzerr;
            f=*addr++; addr+=6;
            if(f&4) { r=*addr++; r+=(*addr++ << 8); addr+=r; }
            if(f&8) { while(*addr++ != 0); }
            if(f&16) { while(*addr++ != 0); }
            if(f&2) addr+=2;
            d.source = addr;
            // allocate destination buffer
            CopyMem(&len,initrd_ptr+initrd_len-4,4);
            uefi_call_wrapper(BS->AllocatePages, 4, 0, 2, (len+PAGESIZE-1)/PAGESIZE, (EFI_PHYSICAL_ADDRESS*)&addr);
            if(addr==NULL)
                return report(EFI_OUT_OF_RESOURCES,L"AllocatePages\n");
            // decompress
            d.bitcount = 0;
            d.bfinal = 0;
            d.btype = -1;
            d.dict_size = 0;
            d.dict_ring = NULL;
            d.dict_idx = 0;
            d.curlen = 0;
            d.dest = addr;
            d.destSize = 1;
            do { r = uzlib_uncompress(&d); } while (!r);
            if (r != TINF_DONE) {
gzerr:          return report(EFI_LOAD_ERROR,L"Corrupted initrd");
            }
            // swap initrd_ptr with the uncompressed buffer
            uefi_call_wrapper(BS->FreePages, 2, (EFI_PHYSICAL_ADDRESS)initrd_ptr, (initrd_len+PAGESIZE-1)/PAGESIZE);
            initrd_ptr=addr;
            initrd_len=len;
        }
        DBG(L" * Initrd loaded @%lx %d bytes\n",initrd_ptr,initrd_len);
        kne=NULL;
        // if there's an environment file, load it
        if(LoadFile(configfile,&env_ptr,&env_len)==EFI_SUCCESS)
            ParseEnvironment(env_ptr,env_len, argc, argv);
        else {
            env_len=0;
            uefi_call_wrapper(BS->AllocatePages, 4, 0, 2, 1, (EFI_PHYSICAL_ADDRESS*)&env_ptr);
            if (env_ptr == NULL) {
                return report(EFI_OUT_OF_RESOURCES,L"AllocatePages");
            }
            ZeroMem((void*)env_ptr,PAGESIZE);
            CopyMem((void*)env_ptr,"// N/A",8);
        }

        // get linear frame buffer
        status = GetLFB();
        if (EFI_ERROR(status) || bootboot->fb_width==0 || bootboot->fb_ptr==0)
                return report(status, L"GOP failed, no framebuffer");

        // collect information on system
        __asm__ __volatile__ (
            "mov $1, %%eax;"
            "cpuid;"
            "shrl $24, %%ebx;"
            "mov %%ebx,%0"
            : : "b"(bootboot->bspid) : "memory" );
        // unlike BIOS bootboot, no need to check if we have
        // PAE + MSR + LME, as we're already in long mode.

        bootboot->protocol=PROTOCOL_STATIC;
        bootboot->loader_type=LOADER_UEFI;
        bootboot->size=128;
        bootboot->pagesize=PAGESIZE;
        CopyMem((void *)&(bootboot->initrd_ptr),&initrd_ptr,8);
        bootboot->initrd_size=((initrd_len+PAGESIZE-1)/PAGESIZE)*PAGESIZE;
        UINT64 p=0xFFFFFFFFFFE00000+(uint64_t)(&(bootboot->mmap))-(uint64_t)(&(bootboot->magic));
        CopyMem(&(bootboot->mmap_ptr),&p,8);
        CopyMem((void *)&(bootboot->efi_ptr),&systab,8);

        // System tables and structures
        LibGetSystemConfigurationTable(&AcpiTableGuid,(void *)&(bootboot->acpi_ptr));
        LibGetSystemConfigurationTable(&SMBIOSTableGuid,(void *)&(bootboot->smbi_ptr));
        LibGetSystemConfigurationTable(&MpsTableGuid,(void *)&(bootboot->mp_ptr));

        // FIX ACPI table pointer on TianoCore...
        ptr = (CHAR8 *)(bootboot->acpi_ptr);
        if(CompareMem(ptr,(const CHAR8 *)"RSDT", 4) && CompareMem(ptr,(const CHAR8 *)"XSDT", 4)) {
            // scan for the real rsd ptr, as AcpiTableGuid returns bad address
            for(i=1;i<256;i++) {
                if(!CompareMem(ptr+i, (const CHAR8 *)"RSD PTR ", 8)){
                    ptr+=i;
                    break;
                }
            }
            // get ACPI system table
            ACPI_RSDPTR *rsd = (ACPI_RSDPTR*)ptr;
            if(rsd->xsdt!=0)
                bootboot->acpi_ptr = rsd->xsdt;
            else
                bootboot->acpi_ptr = (UINT64)((UINT32)rsd->rsdt);
        }

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
        if(bootboot->timezone<-1440||bootboot->timezone>1440)   // TZ in mins
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
            *kne='\n';

        // query size of memory map
        status = uefi_call_wrapper(BS->GetMemoryMap, 5,
            &memory_map_size, memory_map, NULL, &desc_size, NULL);
        if (status!=EFI_BUFFER_TOO_SMALL || memory_map_size==0) {
            return report(EFI_OUT_OF_RESOURCES,L"GetMemoryMap getSize");
        }
        // allocate memory for memory descriptors. We assume that one or two new memory
        // descriptor may created by our next allocate calls and we round up to page size
        memory_map_size+=2*desc_size;
        uefi_call_wrapper(BS->AllocatePages, 4, 0, 2, 
            (memory_map_size+PAGESIZE-1)/PAGESIZE, 
            (EFI_PHYSICAL_ADDRESS*)&memory_map);
        if (memory_map == NULL) {
            return report(EFI_OUT_OF_RESOURCES,L"AllocatePages");
        }

        // create page tables
        uefi_call_wrapper(BS->AllocatePages, 4, 0, 2, 23, (EFI_PHYSICAL_ADDRESS*)&paging);
        if (paging == NULL) {
            return report(EFI_OUT_OF_RESOURCES,L"AllocatePages");
        }
        ZeroMem((void*)paging,23*PAGESIZE);
        DBG(L" * Pagetables PML4 @%lx\n",paging);
        //PML4
        paging[0]=(UINT64)((UINT8 *)paging+4*PAGESIZE)+1;   // pointer to 2M PDPE (16G RAM identity mapped)
        paging[511]=(UINT64)((UINT8 *)paging+PAGESIZE)+1;   // pointer to 4k PDPE (core mapped at -2M)
        //4k PDPE
        paging[512+511]=(UINT64)((UINT8 *)paging+2*PAGESIZE+1);
        //4k PDE
        for(i=0;i<255;i++)
            paging[2*512+256+i]=(UINT64)(((UINT8 *)(bootboot->fb_ptr)+(i<<21))+0x81);   //map framebuffer
        paging[2*512+511]=(UINT64)((UINT8 *)paging+3*PAGESIZE+1);
        //4k PT
        paging[3*512+0]=(UINT64)(bootboot)+1;
        paging[3*512+1]=(UINT64)(env_ptr)+1;
        for(i=0;i<(core_len/PAGESIZE);i++)
            paging[3*512+2+i]=(UINT64)((UINT8 *)core_ptr+i*PAGESIZE+1);
        paging[3*512+511]=(UINT64)((UINT8 *)paging+22*PAGESIZE+1);
        //identity mapping
        //2M PDPE
        for(i=0;i<16;i++)
            paging[4*512+i]=(UINT64)((UINT8 *)paging+(7+i)*PAGESIZE+1);
        //first 2M mapped per page
        paging[7*512]=(UINT64)((UINT8 *)paging+5*PAGESIZE+1);
        for(i=0;i<512;i++)
            paging[5*512+i]=(UINT64)(i*PAGESIZE+1);
        //2M PDE
        for(i=1;i<512*16;i++)
            paging[7*512+i]=(UINT64)((i<<21)+0x81);

        // Get memory map
        int cnt=3;
get_memory_map:
        DBG(L" * Memory Map @%lx %d bytes #%d\n",memory_map, memory_map_size, 4-cnt);
        mmapent=(MMapEnt *)&(bootboot->mmap);
        status = uefi_call_wrapper(BS->GetMemoryMap, 5,
            &memory_map_size, memory_map, &map_key, &desc_size, &desc_version);
        if (EFI_ERROR(status)) {
            return report(status,L"GetMemoryMap");
        }
        last=NULL;
        for(mement=memory_map;mement<memory_map+memory_map_size;mement=NextMemoryDescriptor(mement,desc_size)) {
            // failsafe
            if(bootboot->size>=PAGESIZE || 
                (mement->PhysicalStart==0 && mement->NumberOfPages==0))
                break;
            mmapent->ptr=mement->PhysicalStart;
            mmapent->size=(mement->NumberOfPages*PAGESIZE)+
                ((mement->Type>0&&mement->Type<5)||mement->Type==7?MMAP_FREE:
                (mement->Type==8?MMAP_BAD:
                (mement->Type==9?MMAP_ACPIFREE:
                (mement->Type==10?MMAP_ACPINVS:
                (mement->Type==11||mement->Type==12?MMAP_MMIO:
                MMAP_RESERVED)))));
            // merge continous areas of the same type
            if(last!=NULL && 
                MMapEnt_Type(last) == MMapEnt_Type(mmapent) &&
                MMapEnt_Ptr(last)+MMapEnt_Size(last) == MMapEnt_Ptr(mmapent)){
                    last->size+=MMapEnt_Size(last);
                    mmapent->ptr=mmapent->size=0;
            } else {
                last=mmapent;
                bootboot->size+=16;
                mmapent++;
            }
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
            "xorq %%rsp, %%rsp;"
            "pushq %0;"
            "movq $0x544f4f42544f4f42,%%rax;"    // boot magic 'BOOTBOOT'
            "movq $0xffffffffffe00000,%%rbx;"    // bootboot virtual address
            "movq $0xffffffffffe01000,%%rcx;"    // environment virtual address
            "movq $0xffffffffe0000000,%%rdx;"    // framebuffer virtual address
            "retq"
            : : "a"(entrypoint): "memory" );
    }

    return status;
}

