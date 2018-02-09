/** internal interface for my implementation of FAT32 reading/writing API
 * Author: Lu Tian
 * */

#ifndef _FAT32API_H
#define _FAT32API_H

#include "fat16_32.h"
#include <stdbool.h>
#define MAX_NUM_FILE 128

#define SIZE_FAT_ENTRY 4

typedef struct {
    dirEnt * openedFiles[MAX_NUM_FILE];
    dirEnt * openedFilesParent[MAX_NUM_FILE];
    int device_fd; // the file descriptor of the device file
    dirEnt * curdir; // points to a
    bool initialized;
    unsigned int BytesPerSec;
    unsigned int BytesPerCluster;
    unsigned int startFATSec;
    unsigned int startDataSec;
    unsigned int idxRootDirClus;
    unsigned int FATSz; // size of ONE FAT in sectors
    short numFAT;
} DriverStatus;

extern DriverStatus _status;

void _OS_initialization();

#endif
