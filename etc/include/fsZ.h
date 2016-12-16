/*
 * fsZ.h
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
 * @brief fs/Z filesystem defines and structures
 */

#ifndef _FS_Z_H_
#define _FS_Z_H_

#include <stdint.h>

#define FSZ_VERSION_MAJOR 1
#define FSZ_VERSION_MINOR 0
#define FSZ_SECSIZE 4096

// fid is a file id, a logical sector address of an inode.

/*********************************************************
 *            1st sector, the super block                *
 *********************************************************/
typedef struct {
    uint8_t     reserved[512];      //   0 reserved for loader code
    uint8_t     magic[4];           // 512
    uint8_t     version_major;      // 516
    uint8_t     version_minor;      // 517
    uint8_t     logsec;             // 518 logical sector size, 0=2048,1=4096(default),2=8192...
    uint8_t     physec;             // 519 how many physical sector gives up a logical one
    uint64_t    numsec;             // 520 total number of logical sectors
    uint64_t    numsec_hi;          //      128 bit
    uint64_t    freesec;            // 536 first free sector if fs is defragmented
    uint64_t    freesec_hi;
    uint64_t    rootdirfid;         // 552 logical sector number of root directory's inode
    uint64_t    rootdirfid_hi;
    uint64_t    freesecfid;         // 568 inode to fragmentation records (sector list)
    uint64_t    freesecfid_hi;
    uint64_t    badsecfid;          // 584 inode to bad sectors table (sector list)
    uint64_t    badsecfid_hi;
    uint64_t    indexfid;           // 600 inode to index tables (index file)
    uint64_t    indexfid_hi;
    uint64_t    metadatafid;        // 616 inode to label meta data (label database)
    uint64_t    metadatafid_hi;
    uint32_t    owneruuid[4];       // 632 owner UUID
    uint16_t    maxmounts;          // 648 number of maximum mounts allowed
    uint16_t    currmounts;         // 650 current mount counter
    uint8_t     createdate[8];      // 652
    uint8_t     lastmountdate[8];   // 660
    uint8_t     lastcheckdate[8];   // 668
    uint8_t     lastdefragdate[8];  // 676
    uint8_t     raidtype;           // 684
    uint8_t     flags[7];           // 685
    uint8_t     reserved2[68+256];
    uint8_t     magic2[4];          //1016
    uint32_t    checksum;           //1020 Castagnoli CRC32 of bytes at 512-1020
    uint8_t     raidspecific[FSZ_SECSIZE-1024];
} __attribute__((packed)) FSZ_SuperBlock;

#define FSZ_MAGIC "FS/Z"

#define FSZ_SB_FLAG_HIST   (1<<0)   // indicates that previous file versions are kept
#define FSZ_SB_SOFTRAID_NONE      0    // single disk
#define FSZ_SB_SOFTRAID0          1    // mirror
#define FSZ_SB_SOFTRAID1          2    // concatenate
#define FSZ_SB_SOFTRAID5          3    // xored blocks

/*********************************************************
 *                    I-node sector                      *
 *********************************************************/
// fid: file id, logical sector number where the sector
//      contains an inode.
//sizeof = 32
typedef struct {
    uint64_t    fid;
    uint64_t    fid_hi;
    uint64_t    size;
    uint64_t    size_hi;
} __attribute__((packed)) FSZ_SectorList;
// used at several places, like free and bad block list in
// superblock or data locators in inodes.

//sizeof = 16, one Access Control Entry
typedef struct {
    uint8_t     type;               // access rights
    uint8_t     uuid[15];           // owner UUID without the first byte
} __attribute__((packed)) FSZ_Access;

#define FSZ_READ    (1<<0)
#define FSZ_WRITE   (1<<1)
#define FSZ_EXEC    (1<<2)          // execute or search
#define FSZ_APPEND  (1<<3)
#define FSZ_DELETE  (1<<4)
#define FSZ_SUID    (1<<6)          // Set user id on execution
#define FSZ_SGID    (1<<7)          // Inherit ACL, no groups per se in OS/Z

// if inode.fid == inode.sec => inline data
//sizeof = 4096
typedef struct {
    uint8_t     magic[4];       //   0
    uint32_t    checksum;       //   4 Castagnoli CRC32, filetype to end of the sector
    uint8_t     filetype[4];    //   8 first 4 bytes of mime main type, eg: text,imag,vide,audi,appl etc.
    uint8_t     mimetype[48];   //  12 mime sub type, eg.: plain, html, gif, jpeg etc.
    uint8_t     encrypt[20];    //  60 AES encryption key part or zero
    uint64_t    numlinks;       //  80 number of fids to this inode
    uint8_t     createdate[8];  //  88 number of seconds since 1970. jan. 1 00:00:00 UTC
    uint8_t     lastaccess[8];  //  96
    uint8_t     reserved[24];   // 104 padding
    // FSZ_Version oldversions[5];
    uint8_t     version5[64];   // 128 previous versions if enabled
    uint8_t     version4[64];   // 192 all the same format as the current
    uint8_t     version3[64];   // 256
    uint8_t     version2[64];   // 320
    uint8_t     version1[64];   // 384
    // FSZ_Version current;
    uint64_t        sec;        // 448 current (or only) version
    uint64_t        sec_hi;
    uint64_t        size;       // 464 file size
    uint64_t        size_hi;
    FSZ_Access      owner;      // 480
    uint64_t        modifydate; // 496
    uint32_t        filechksum; // 504 Castignioli CRC32 of data
    uint32_t        flags;      // 508
    FSZ_Access  groups[32];     // 512 List of 32 FSZ_Access entries
    uint8_t     inlinedata[FSZ_SECSIZE-1024];
} __attribute__((packed)) FSZ_Inode;

#define FSZ_IN_MAGIC "FSIN"

// regular files, 4th character never ':'
#define FILETYPE_REG_TEXT   "text"  // main part of mime type
#define FILETYPE_REG_IMAGE  "imag"
#define FILETYPE_REG_VIDEO  "vide"
#define FILETYPE_REG_AUDIO  "audi"
#define FILETYPE_REG_APP    "appl"
// special entities, 4th character always ':'
#define FILETYPE_DIR        "dir:"  // see below
#define FILETYPE_SECLST     "lst:"  // for free and bad sector lists
#define FILETYPE_INDEX      "idx:"  // search cache, not implemented yet
#define FILETYPE_META       "mta:"  // meta labels, not implemented yet
#define FILETYPE_CHARDEV    "chr:"  // character device
#define FILETYPE_BLKDEV     "blk:"  // block device
#define FILETYPE_FIFO       "fio:"  // First In First Out queue
#define FILETYPE_SOCK       "sck:"  // socket
#define FILETYPE_MOUNT      "mnt:"  // mount point

// logical sector address to data sector translation. These file sizes
// were calculated with 4096 sector size. That is configurable in the
// FSZ_SuperBlock if you think 11 sector reads is too much to access
// data at any arbitrary position in a Yotta magnitude file.

/*  any data size
    FSZ_Inode.sec points to sector with FSZ_SectorList
    entries
    FSZ_Inode.sec -> sl -> data */
#define FSZ_IN_FLAG_SECLIST (0xFE<<0)

/*  data size < sector size - 1024 (3072 bytes)
    FSZ_Inode.sec points to itself.
    the data is included in the inode sector
    FSZ_Inode.sec -> FSZ_Inode.sec  */
#define FSZ_IN_FLAG_INLINE  (0xFF<<0)

/*  data size < sector size (4096)
    The inode points to data sector directly
    FSZ_Inode.sec -> data */
#define FSZ_IN_FLAG_DIRECT  (0<<0)

/*  data size < sector size * sector size / 16 (1 M)
    FSZ_Inode.sec points to a sector directory,
    which is a sector with up to 512 sector
    addresses
    FSZ_Inode.sec -> sd -> data */
#define FSZ_IN_FLAG_SD      (1<<0)

/*  data size < sector size * sector size / 16 * sector size / 16 (256 M)
    FSZ_Inode.sec points to a sector directory,
    which is a sector with up to 512 sector
    directory addresses, which in turn point
    to 512*512 sector addresses
    FSZ_Inode.sec -> sd -> sd -> data */
#define FSZ_IN_FLAG_SD2     (2<<0)

/*  data size < (64 G)
    FSZ_Inode.sec -> sd -> sd -> sd -> data */
#define FSZ_IN_FLAG_SD3     (3<<0)

/*  data size < (16 T)
    FSZ_Inode.sec -> sd -> sd -> sd -> sd -> data */
#define FSZ_IN_FLAG_SD4     (4<<0)

/*  data size < (4 Peta, equals 4096 Terra)
    FSZ_Inode.sec -> sd -> sd -> sd -> sd -> sd -> data */
#define FSZ_IN_FLAG_SD5     (5<<0)

/*  data size < (1 Exa, equals 1024 Peta)
    FSZ_Inode.sec -> sd -> sd -> sd -> sd -> sd -> sd -> data */
#define FSZ_IN_FLAG_SD6     (6<<0)

/*  data size < (256 Exa)
    FSZ_Inode.sec -> sd -> sd -> sd -> sd -> sd -> sd -> sd -> data */
#define FSZ_IN_FLAG_SD7     (7<<0)

/*  data size < (64 Zetta, equals 65536 Exa) */
#define FSZ_IN_FLAG_SD8     (8<<0)

/*  data size < (16 Yotta, equals 16384 Zetta) */
#define FSZ_IN_FLAG_SD9     (9<<0)

/*********************************************************
 *                      Directory                        *
 *********************************************************/
//sizeof = 128
typedef struct {
    uint8_t     magic[4];
    uint32_t    checksum;       // Castagnoli CRC32 of entries
    uint64_t    numentries;
    uint32_t    flags;
    uint8_t     reserved[108];
} __attribute__((packed)) FSZ_DirEntHeader;

#define FSZ_DIR_MAGIC "FSDR"
#define FSZ_DIR_FLAG_UNSORTED (1<<0)

//directory entries are lexicographically ordered. It means
//a bit slower writes, but also means incredibly faster look ups.

//sizeof = 128
typedef struct {
    uint64_t    fid;
    uint64_t    fid_hi;
    uint8_t     length;             // number of UNICODE characters in name
    uint8_t     name[111];          // zero terminated UTF-8 string
} __attribute__((packed)) FSZ_DirEnt;

#endif
