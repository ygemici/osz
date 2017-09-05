/*
 * fsZ.h
 *
 * 2016 Public Domain bztsrc@github
 * 
 * IMPORTANT NOTE: the on disk format of FS/Z is public domain, you
 * can use it without any restrictions and free of charge. The right
 * to create and use disks and images with FS/Z and implement programs
 * to do so is hereby granted to everybody. The filesystem driver for
 * it in OS/Z on the other hand licensed under CC-by-nc-sa.
 *
 * @brief FS/Z filesystem defines and structures for on disk format
 */

#ifndef _FS_Z_H_
#define _FS_Z_H_

#include <stdint.h>
#include <sys/types.h>

#define FSZ_VERSION_MAJOR 1
#define FSZ_VERSION_MINOR 0
#define FSZ_SECSIZE 4096

// fid is a file id, a logical sector address of an inode. Logical sector addresses
// are 128 bits long to be future proof, although currently only 64 bits used. That
// gives you to total capacity of 64 Zettabytes current implementation can handle with
// 4096 bytes sector size.

/*********************************************************
 *            1st sector, the super block                *
 *********************************************************/
typedef struct {
    uint8_t     loader[512];        //   0 reserved for loader code
    uint8_t     magic[4];           // 512
    uint8_t     version_major;      // 516
    uint8_t     version_minor;      // 517
    uint8_t     flags;              // 518 flags
    uint8_t     raidtype;           // 519 raid type
    uint64_t    numsec;             // 520 total number of logical sectors
    uint64_t    numsec_hi;          //      128 bit
    uint64_t    freesec;            // 536 first free sector if fs is defragmented, otherwise
    uint64_t    freesec_hi;         //     it's the last used sector+1
    uint64_t    rootdirfid;         // 552 logical sector number of root directory's inode
    uint64_t    rootdirfid_hi;
    uint64_t    freesecfid;         // 568 inode to free records (FSZ_SectorList allocated)
    uint64_t    freesecfid_hi;
    uint64_t    badsecfid;          // 584 inode to bad sectors table (FSZ_SectorList allocated)
    uint64_t    badsecfid_hi;
    uint64_t    indexfid;           // 600 inode to search index. Zero if not indexed
    uint64_t    indexfid_hi;
    uint64_t    metafid;            // 616 inode to meta labels file. Zero if there's no meta label
    uint64_t    metafid_hi;
    uint64_t    journalfid;         // 632 inode to journal file. Zero if journaling is turned off
    uint64_t    journalfid_hi;
    uint64_t    journalstr;         // 648 logical sector inside journal file where buffer starts
    uint64_t    journalstr_hi;
    uint64_t    journalend;         // 664 logical sector inside journal file where buffer ends
    uint64_t    journalend_hi;
    uint8_t     encrypt[20];        // 680 encryption mask for AES or zero
    uint32_t    enchash;            // 700 password Castagnoli CRC32, to avoid decryption with bad passwords
    uint16_t    maxmounts;          // 704 number of maximum mounts allowed to next fsck
    uint16_t    currmounts;         // 706 current mount counter
    uint16_t    logsec;             // 708 logical sector size, 0=2048,1=4096(default),2=8192...
    uint16_t    physec;             // 710 how many physical sector gives up a logical one, defaults to 8
    uint64_t    createdate;         // 712 creation timestamp UTC
    uint64_t    lastmountdate;      // 720
    uint64_t    lastcheckdate;      // 728
    uint64_t    lastdefragdate;     // 736
    uint32_t    owneruuid[4];       // 744 owner UUID
    uint8_t     reserved[256];
    uint8_t     magic2[4];          //1016
    uint32_t    checksum;           //1020 Castagnoli CRC32 of bytes at 512-1020
    uint8_t     raidspecific[FSZ_SECSIZE-1024];
} __attribute__((packed)) FSZ_SuperBlock;

#define FSZ_MAGIC "FS/Z"

#define FSZ_SB_FLAG_HIST      (1<<0)   // indicates that previous file versions are kept
#define FSZ_SB_FLAG_BIGINODE  (1<<1)   // indicates inode size is 2048 (ACL size 96 instead of 32)

#define FSZ_SB_SOFTRAID_NONE   0xff    // single disk
#define FSZ_SB_SOFTRAID0          0    // mirror
#define FSZ_SB_SOFTRAID1          1    // concatenate
#define FSZ_SB_SOFTRAID5          5    // xored blocks

/* Encryption: the mask is xored with the hash(password+salt) to get the AES key. This way
 * the encrypted disk doesn't need to be rewritten when password changes. Also you can
 * re-encrypt the disk without changing the password. */
 
/*********************************************************
 *                    I-node sector                      *
 *********************************************************/
// fid: file id, logical sector number where the sector contains an inode.

//sizeof = 32
typedef struct {
    uint64_t    fid;
    uint64_t    fid_hi;
    uint64_t    numsec;
    uint64_t    numsec_hi;
} __attribute__((packed)) FSZ_SectorList;
// used at several places, like free and bad block list inodes.

//sizeof = 16, one Access Control Entry, UUID without the last byte
typedef struct {
    uint8_t uuid[15];
    uint8_t access;
} __attribute__((packed)) FSZ_Access;

//access rights are stored in the last byte. Make sure this matches
//the system access flags A_* in sys/types.h
#define FSZ_READ    (1<<0)
#define FSZ_WRITE   (1<<1)
#define FSZ_EXEC    (1<<2)          // execute or search
#define FSZ_APPEND  (1<<3)
#define FSZ_DELETE  (1<<4)
#define FSZ_SUID    (1<<6)          // Set user id on execution
#define FSZ_SGID    (1<<7)          // Inherit ACL, no set group per se in OS/Z

//file version structure. You can use this to point to version5, version4 etc.
//sizeof = 64
typedef struct {
    uint64_t    sec;
    uint64_t    sec_hi;
    uint64_t    size;
    uint64_t    size_hi;
    uint64_t    modifydate;
    uint32_t    filechksum;
    uint32_t    flags;
    FSZ_Access  owner;
} __attribute__((packed)) FSZ_Version;

//sizeof = 4096
typedef struct {
    uint8_t     magic[4];       //   0 magic 'FSIN'
    uint32_t    checksum;       //   4 Castagnoli CRC32, filetype to end of the sector
    uint8_t     filetype[4];    //   8 first 4 bytes of mime main type, eg: text,imag,vide,audi,appl etc.
    uint8_t     mimetype[52];   //  12 mime sub type, eg.: plain, html, gif, jpeg etc.
    uint8_t     encrypt[20];    //  64 AES encryption key mask or zero
    uint32_t    enchash;        //  84 password Castagnoli CRC32, to avoid decryption with bad passwords
    uint64_t    createdate;     //  88 number of seconds since 1970. jan. 1 00:00:00 UTC
    uint64_t    lastaccess;     //  96
    uint64_t    metalabel;      // 104 sector offset into meta labels file
    uint64_t    metalabel_hi;
    uint64_t    numlinks;       // 120 number of references to this inode
    FSZ_Version version5;       // 128 previous oldest version (if versioning enabled)
    FSZ_Version version4;       // 192 all the same format as the current
    FSZ_Version version3;       // 256 see FSZ_Version structure above
    FSZ_Version version2;       // 320
    FSZ_Version version1;       // 384
    // FSZ_Version current; I haven't used FSZ_Version struct here to save typing when referencing
    uint64_t         sec;       // 448 current (or only) version
    uint64_t         sec_hi;
    uint64_t         size;      // 464 file size
    uint64_t         size_hi;
    uint64_t         modifydate;// 480
    uint32_t         filechksum;// 488 Castagnoli CRC32 of data
    uint32_t         flags;     // 492 see FSZ_IN_FLAG_*
    //owner is the last in FSZ_Version to followed by ACL.
    //so first entry in ACL is the control ACE
    FSZ_Access       owner;     // 496
    union {
        struct {
          FSZ_Access groups[32];// 512 List of 32 FSZ_Access entries, groups
          uint8_t    inlinedata[FSZ_SECSIZE-1024];
        };
        struct {
          FSZ_Access groups[96];// 512 List of 96 FSZ_Access entries, groups
          uint8_t    inlinedata[FSZ_SECSIZE-2048];
        } big;
    };
} __attribute__((packed)) FSZ_Inode;

#define FSZ_IN_MAGIC "FSIN"

// regular files, 4th character never ':'
#define FILETYPE_REG_TEXT   "text"  // main part of mime type
#define FILETYPE_REG_IMAGE  "imag"
#define FILETYPE_REG_VIDEO  "vide"
#define FILETYPE_REG_AUDIO  "audi"
#define FILETYPE_REG_APP    "appl"
// special entities, 4th character always ':'
#define FILETYPE_DIR        "dir:"  // directory
#define FILETYPE_UNION      "uni:"  // directory union, inlined data is a zero separated list of paths with jokers
#define FILETYPE_INTERNAL   "int:"  // internal files
#define FILETYPE_SYMLINK    "url:"  // symbolic link, inlined data is a path
// mime types for filesystem specific files
// for FILETYPE_DIR
#define MIMETYPE_DIR_ROOT   "fs-root"  // root directory (for recovery it has a special mime type)
// for FILETYPE_INTERNAL
#define MIMETYPE_INT_FREELST "fs-free-sectors" // for free sector list
#define MIMETYPE_INT_BADLST  "fs-bad-sectors"  // for bad sector list
#define MIMETYPE_INT_META    "fs-meta-labels"  // meta labels
#define MIMETYPE_INT_INDEX   "fs-search-index" // search cache
#define MIMETYPE_INT_JOURNAL "fs-journal-data" // journaling records

// logical sector address to data sector translation. These file sizes
// were calculated with 4096 sector size. That is configurable in the
// FSZ_SuperBlock if you think 11 sector reads is too much to access
// data at any arbitrary position in a Yotta magnitude file.

// if even that's not enough, you can use sector lists (extents) to store file data.

#define FLAG_TRANSLATION(x) ((x>>0)&0xFF)

/*  data size < sector size - 1024 (3072 bytes)
    FSZ_Inode.sec points to itself.
    the data is included in the inode sector at 1024 (or 2048 if BIGINODE)
    FSZ_Inode.sec -> FSZ_Inode.sec  */
#define FSZ_IN_FLAG_INLINE  (0xFF<<0)

/*  data size < sector size (4096)
    The inode points to data sector directly
    FSZ_Inode.sec -> data */
#define FSZ_IN_FLAG_DIRECT  (0<<0)

/*  data size < sector size * sector size / 16 (1 M)
    FSZ_Inode.sec points to a sector directory,
    which is a sector with up to 256 sector addresses
    FSZ_Inode.sec -> sd -> data */
#define FSZ_IN_FLAG_SD      (1<<0)

/*  data size < sector size * sector size / 16 * sector size / 16 (256 M)
    FSZ_Inode.sec points to a sector directory,
    which is a sector with up to 256 sector
    directory addresses, which in turn point
    to 256*256 sector addresses
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

/*  inlined sector list (up to 96 entries)
    FSZ_Inode.sec points to itself, FSZ_SectorList entries inlined.
    FSZ_Inode.sec -> FSZ_Inode.sec -> sl -> data */
#define FSZ_IN_FLAG_SECLIST  (0x80<<0)

/*  normal sector list (up to 128 entries)
    FSZ_Inode.sec points to a sector with FSZ_SectorList entries.
    FSZ_Inode.sec -> sl -> data */
#define FSZ_IN_FLAG_SECLIST0 (0x81<<0)

/*  indirect sector list (up to 32768 entries)
    FSZ_Inode.sec points to a sector directory with FSZ_SectorLists
    FSZ_Inode.sec -> sd -> sl -> data */
#define FSZ_IN_FLAG_SECLIST1 (0x82<<0)

/*  double-indirect sector list (up to 8388608 entries)
    FSZ_Inode.sec points to a sector directory pointing to
    sector directories with FSZ_SectorLists
    FSZ_Inode.sec -> sd -> sd -> sl -> data */
#define FSZ_IN_FLAG_SECLIST2 (0x83<<0)

/*  triple-indirect sector list (up to 2147483648 entries)
    FSZ_Inode.sec -> sd -> sd -> sd -> sl -> data */
#define FSZ_IN_FLAG_SECLIST3 (0x84<<0)

/*********************************************************
 *                      Directory                        *
 *********************************************************/
// first entry is the header.

//sizeof = 128
typedef struct {
    uint8_t     magic[4];
    uint32_t    checksum;       // Castagnoli CRC32 of entries
    uint64_t    numentries;
    uint64_t    numentries_hi;
    uint32_t    flags;
    uint8_t     reserved[100];
} __attribute__((packed)) FSZ_DirEntHeader;

#define FSZ_DIR_MAGIC "FSDR"
#define FSZ_DIR_FLAG_UNSORTED (1<<0)

//directory entries are fixed in size and lexicographically ordered.
//this means a bit slower writes, but also incredibly faster look ups.

//sizeof = 128
typedef struct {
    uint64_t      fid;
    uint64_t      fid_hi;
    uint8_t       length;             // number of UNICODE characters in name
    unsigned char name[111];          // zero terminated UTF-8 string
} __attribute__((packed)) FSZ_DirEnt;

/*********************************************************
 *                   Directory union                     *
 *********************************************************/

/* inlined data is a list of asciiz paths that may contain the '*' joker character.
 * Terminated by and empty path.*/
 // Example union for /usr/bin: inlinedata=/bin(zero)/usr/*/bin(zero)(zero)

/*********************************************************
 *                     Meta labels                       *
 *********************************************************/

/* meta labels are list of sector aligned, zero terminated JSON strings,
 * filled up with zeros to be multiple of sector size. The metalabel sector
 * in inode points to the start position.
 *
 * Example (assuming meta label file is continous and starts at lba 1234):
 * {"icon":"/usr/firefox/share/icon.png"} (zeros padding to sector size)
 * {"icon":"/usr/vlc/share/icon.svg","downloaded":"http://videolan.org"} (zeros, at least one)
 *
 * Inode of /usr/firefox/bin/firefox: metalabel=1234
 * Inode of /usr/vlc/bin/vlc: metalabel=1235
 */

/*********************************************************
 *                    Search index                       *
 *********************************************************/

/* to be specified. Inode lists for search keywords ("search" meta labels) */

/*********************************************************
 *                    Journal data                       *
 *********************************************************/

/* to be specified. Circular buffer of modified sectors */

#endif /* fsZ.h */
