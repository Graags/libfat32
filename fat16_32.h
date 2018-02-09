/*
  data structure and interface for FAT16/32 file system and reading/writing API
  Author: Lu Tian
*/
#ifndef _FAT16_32_H
#define _FAT16_32_H
#include <stdint.h>
typedef struct{
    unsigned char jmpBoot[3];// = {0xEB, 0x00, 0x90};
    unsigned char OEMName[8];
    unsigned short BytsPerSec;
    unsigned char SecPerClus;
    unsigned short RsvdSecCnt;//reserved sector count
    unsigned char NumFATs;//number of FAT
    unsigned short RootEntCnt;//number of root
    unsigned short TotSec16;
    unsigned char Media;
    unsigned short FATSz16;
    unsigned short SecPerTrk;
    unsigned short NumHeads;
    unsigned int HiddSec;
    unsigned int TotSec32;
} __attribute__((packed)) BPB_common;


typedef struct {
    BPB_common bpb_common;
    unsigned char DrvNum;
    unsigned char Reserved1;
    unsigned char BootSig;
    unsigned int VolID;
    unsigned char VolLab[11];
    unsigned char FilSysType[8];
    unsigned char remain[450];
} __attribute__((packed)) FAT16_BPB;

typedef struct {
    BPB_common bpb_common;
    unsigned int FATSz32;
    unsigned short ExtFlags;
    unsigned short FSVer;
    unsigned int RootClus;
    unsigned short FSInfo;
    unsigned short BkBootSec;
    unsigned char Reserved[12];
    unsigned char DrvNum;
    unsigned char Reserved1;
    unsigned char BootSig;
    unsigned int VolID;
    unsigned char VolLab[11];
    unsigned char FilSysType[8];
    unsigned char remain[422];
} __attribute__((packed)) FAT32_BPB;


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

extern int OS_read(int fd, void *buf, int nbyte, int offset);

extern dirEnt * OS_readDir(const char * dirname);

extern int OS_mkdir(const char *path);

extern int OS_rmdir(const char *path);

extern int OS_rm(const char *path); // remove a file

extern int OS_creat(const char *path);

extern int OS_write(int fildes, const void *buf, int nbytes, int offset);
// write to file specified by fildes

#endif
