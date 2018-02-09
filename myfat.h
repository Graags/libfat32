#ifndef __MYFAT_H__
#define __MYFAT_H__

typedef struct __attribute__ ((packed)) {

        uint8_t bs_jmpBoot[3];          // jmp instr to boot code
        uint8_t bs_oemName[8];          // indicates what system formatted this field, default=MSWIN4.1
        uint16_t bpb_bytesPerSec;       // Count of bytes per sector
        uint8_t bpb_secPerClus;         // no.of sectors per allocation unit
        uint16_t bpb_rsvdSecCnt;        // no.of reserved sectors in the resercved region of the volume starting at 1st sector
        uint8_t bpb_numFATs;            // The count of FAT datastructures on the volume
        uint16_t bpb_rootEntCnt;        // Count of 32-byte entries in root dir, for FAT32 set to 0
        uint16_t bpb_totSec16;          // total sectors on the volume
        uint8_t bpb_media;              // value of fixed media
        uint16_t bpb_FATSz16;           // count of sectors occupied by one FAT
        uint16_t bpb_secPerTrk;         // sectors per track for interrupt 0x13, only for special devices
        uint16_t bpb_numHeads;          // no.of heads for intettupr 0x13
        uint32_t bpb_hiddSec;           // count of hidden sectors
        uint32_t bpb_totSec32;          // count of sectors on volume
        uint32_t bpb_FATSz32;           // define for FAT32 only
        uint16_t bpb_extFlags;          // Reserved for FAT32
        uint16_t bpb_FSVer;             // Major/Minor version num
        uint32_t bpb_RootClus;          // Clus num of 1st clus of root dir
        uint16_t bpb_FSInfo;            // sec num of FSINFO struct
        uint16_t bpb_bkBootSec;         // copy of boot record
        uint8_t bpb_reserved[12];       // reserved for future expansion
        uint8_t bs_drvNum;              // drive num
        uint8_t bs_reserved1;           // for ue by NT
        uint8_t bs_bootSig;             // extended boot signature
        uint32_t bs_volID;              // volume serial number
        uint8_t bs_volLab[11];          // volume label
        uint8_t bs_fileSysTye[8];       // FAT12, FAT16 etc
} bpbFat32 ;

typedef struct __attribute__ ((packed)) {
        uint8_t dir_name[11];           // short name
        uint8_t dir_attr;               // File sttribute
        uint8_t dir_NTRes;              // Set value to 0, never chnage this
        uint8_t dir_crtTimeTenth;       // millisecond timestamp for file creation time
        uint16_t dir_crtTime;           // time file was created
        uint16_t dir_crtDate;           // date file was created
        uint16_t dir_lstAccDate;        // last access date
        uint16_t dir_fstClusHI;         // high word fo this entry's first cluster number
        uint16_t dir_wrtTime;           // time of last write
        uint16_t dir_wrtDate;           // dat eof last write
        uint16_t dir_fstClusLO;         // low word of this entry's first cluster number
        uint32_t dir_fileSize;          // 32-bit DWORD hoding this file's size in bytes
} dirEnt;


extern int OS_cd(const char *path);
extern int OS_open(const char *path);
extern int OS_close(int fd);
extern int OS_read(int fildes, void *buf, int nbyte, int offset);
extern dirEnt * OS_readDir(const char *dirname);

extern int OS_mkdir(const char *path);

extern int OS_rmdir(const char *path);

extern int OS_rm(const char *path); // remove a file

extern int OS_creat(const char *path);

extern int OS_write(int fildes, const void *buf, int nbytes, int offset);


char *cwdPath;          // current working dir name
int fdCount; 

#endif
