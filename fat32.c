/*
 FAT32 file system reading and writing APIs
 CS6456 Operating System Homework 3-4
 Fall 2017

 Author: Lu Tian
 fat32.c - implementation of reading APIs
 The following code implements all FAT32 read and write APIs:

 OS_cd(const char *dirname): change current working directory to dirname
 OS_open(const char *path): open file "path"
 OS_close(int fd): close file specified by "fd"
 OS_read(int fd, void* buffer,  int length): read the function
     specified by "fd"
 OS_readDir(const char *dirname): get the list of content of directory
     "dirname"
 OS_mkdir(const char *dirname): create the director with name @dirname
 OS_creat(const char *filename): create the file with name @filename
 OS_write(int fd, void * buffer, int nbytes, int offset): write the content
  * of @buffer to file with descriptor @fd
 OS_rmdir(const char *dirname): remove directory @dirname
 OS_rm(const char *filename): remove file @filename
 */

#include "fat16_32.h"
#include "fat32api.h"
#include "utils32.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

DriverStatus _status =
{.openedFiles = {NULL},
.initialized = false,
.device_fd = 0,
.curdir = NULL,
.FATSz = 0
};

void _OS_initialization() {
    char *strEnv = getenv("FAT_FS_PATH"); // get the environment variable, path to the file
    _status.device_fd = open(strEnv, O_RDWR, 0);
    FAT32_BPB *bpb_info = (FAT32_BPB *)malloc(sizeof(FAT32_BPB));
    read(_status.device_fd, (void *)bpb_info, sizeof(FAT32_BPB) ); // read the BPB info from disk

    // Calculate parameters for convenience
    _status.BytesPerSec = bpb_info->bpb_common.BytsPerSec;
    _status.BytesPerCluster = _status.BytesPerSec *
        (unsigned short)bpb_info->bpb_common.SecPerClus;
    //printf("%d bytes per cluster.\n", _status.BytesPerCluster);
    _status.startFATSec = bpb_info->bpb_common.RsvdSecCnt;
    _status.numFAT = bpb_info->bpb_common.NumFATs;
    _status.FATSz = bpb_info->FATSz32;
    _status.startDataSec = _status.startFATSec + bpb_info->bpb_common.NumFATs * bpb_info->FATSz32;
    _status.idxRootDirClus = bpb_info->RootClus;

    _status.curdir = malloc(sizeof(dirEnt)); // there is no dirEnt for root directory
    _initVirtualRootDirEnt(_status.curdir); // so we make up a virtual dirEnt for root
    _status.initialized = true;
    memset(_status.openedFiles, 0, sizeof(dirEnt *) * MAX_NUM_FILE);
    free(bpb_info);
    /* initialize all pointers to NULL*/
    return;
}


int OS_cd(const char *path) {
    //if not initialized, initialize
    if(_status.initialized != 1) _OS_initialization();
    dirEnt *ptr = _OS_getEnt(path);
    if(!ptr) return -1;
    free(_status.curdir);
    // in all time, cur points to an allocated memory, so we need to free it
    // when it is covered
    _status.curdir = ptr;
    return 1;
}

int OS_open(const char *path){
    if(_status.initialized != 1) _OS_initialization();
    dirEnt *ptr = _OS_getEnt(path);

    /// get the dirEnt of parent dir
    char * parent_path = _get_parent_path(path, NULL);
    dirEnt *parent_ptr = _OS_getEnt(parent_path);
    free(parent_path);

    if(!ptr) return -1;
    int fd = 0;
    for( ; fd < MAX_NUM_FILE; fd++){
        if(_status.openedFiles[fd] == NULL){
            _status.openedFiles[fd] = ptr;// find the first available file descriptor
            _status.openedFilesParent[fd] = parent_ptr;
            return fd;
        }
    }
    free(ptr);
    return -1;
}

int OS_close(int fd){
    if(_status.initialized != 1) _OS_initialization();
    if(_status.openedFiles[fd]==NULL){
        return -1;
    } else {
        free(_status.openedFiles[fd]);
        free(_status.openedFilesParent[fd]);
        _status.openedFiles[fd] = NULL;
        _status.openedFilesParent[fd] = NULL;
        return 1;
    }
}


int OS_read(int fd, void *buf, int nbyte, int offset){
    if(_status.initialized != 1) _OS_initialization();
    /** get the first cluster idx */
    if(_status.openedFiles[fd] == NULL) {
        return -1;
    }
    int nbyte_really = nbyte; // number of bytes really read
    if(nbyte + offset > _status.openedFiles[fd]->dir_fileSize){
        nbyte_really = (int)(_status.openedFiles[fd]->dir_fileSize) - offset;
        if(nbyte_really < 0) return -1;
    }
    unsigned int fstCluster = _status.openedFiles[fd]->dir_fstClusHI * 65536
        + _status.openedFiles[fd]->dir_fstClusLO;
    return _OS_read_file(fstCluster, offset, nbyte_really, buf);
}

dirEnt * OS_readDir(const char * dirname) {
    if(_status.initialized != 1) _OS_initialization();
    dirEnt * ent = _OS_getEnt(dirname);
    if(ent == NULL) return NULL;
    if(ent->dir_attr != 0x10) {
        free(ent);
        return NULL;
    }

    const int numDirEntPerClus = _status.BytesPerCluster / sizeof(dirEnt);
    dirEnt * dirEntBuf = calloc(_status.BytesPerCluster, 1);
    int volBuffer = _status.BytesPerCluster / sizeof(dirEnt);
    // volume of this buffer, in dirEnt
    int sizeBuffer = 0; // occupied size, in dirEnt
    unsigned int startCluster = 0;
    if(ent->dir_fstClusLO==0 && ent->dir_fstClusHI==0) {//if this is the root
        startCluster = _status.idxRootDirClus;
    } else {
        startCluster = ent->dir_fstClusLO + ent->dir_fstClusHI * 65536;
    }
    bool endDetected = false;
    int i = 0;
    while(!endDetected)
    {
        while(sizeBuffer + numDirEntPerClus > volBuffer)   // resize the buffer
        {
            dirEntBuf = realloc(dirEntBuf, 2 * volBuffer * sizeof(dirEnt));
            memset(dirEntBuf + volBuffer, 0, volBuffer * sizeof(dirEnt));
            volBuffer *= 2;
        }
        _OS_read_file(startCluster, i * _status.BytesPerCluster, _status.BytesPerCluster, dirEntBuf + sizeBuffer);
        int j;
        for(j = sizeBuffer; j < sizeBuffer + numDirEntPerClus; j++)
        {
            if(dirEntBuf[j].dir_name[0] == '\0')
            {
                endDetected = true;
                break;
            }
        }
        sizeBuffer += numDirEntPerClus;
        i++;
    }

    free(ent);
    return dirEntBuf;
}

int OS_mkdir(const char * path) {
    if(!_status.initialized)_OS_initialization();

    // first check whether this dir already exists
    dirEnt *self_dirEnt = _OS_getEnt(path);
    // if the file/dir already exists, return with error
    if(self_dirEnt != NULL) {
        free(self_dirEnt);
        return -2;
    }
    // first get the path to the parent dir
    char filename_buffer[12];
    char * parentdir = _get_parent_path(path, filename_buffer);
    if(!parentdir) { // cannot find parent dir
        return -1; // path invalid
    }

    // Then we get the dirEnt of parent dir
    dirEnt * parent_dirEnt = _OS_getEnt(parentdir);

    // if parent dir not found, return with error
    if(parent_dirEnt == NULL) {return -1;}

    // get the list of parent dir, then we can get how many entries are already used
    dirEnt * list_parent_dirEnt = OS_readDir(parentdir);
    int i = 0;
    while(list_parent_dirEnt[i].dir_name[0]!=0 && list_parent_dirEnt[i].dir_name[0] != 0xE5) i++;
    //now i is the number of existing entries
    // if this is the last dirEnt, then write 2 dirEnts (with an empty one)
    // if this is not the last one (starting with 0xE5), the write one
    int num_ent_write = list_parent_dirEnt[i].dir_name[0] == 0xE5 ? 1 : 2;
    free(list_parent_dirEnt); list_parent_dirEnt = NULL;
    
    //get a new cluster
    uint32_t new_clus_idx = _findFirstEmptyClus();
    dirEnt append_ent[3];
    memset(append_ent, 0, 3 * sizeof(dirEnt));

    //set the attributes
    append_ent[0].dir_attr = 0x10;
    append_ent[0].dir_wrtTime = 0;
    append_ent[0].dir_wrtDate = 0;
    append_ent[0].dir_fstClusLO = new_clus_idx & 0xFFFF;
    append_ent[0].dir_fstClusHI = new_clus_idx >> 16;
    append_ent[0].dir_fileSize = 0;
    flnm2FAT(filename_buffer,append_ent[0].dir_name);
    //TODO: set the time and date


    ///write the appended dirEnts to parent dirEnt
    if(parent_dirEnt->dir_fstClusLO == 0 && parent_dirEnt->dir_fstClusHI == 0) {
        _OS_write_file(_status.idxRootDirClus , append_ent, num_ent_write*sizeof(dirEnt),
                       i*sizeof(dirEnt));
    } else{
        _OS_write_file(parent_dirEnt->dir_fstClusLO + ((uint32_t)parent_dirEnt->dir_fstClusHI << 16),
                   append_ent, num_ent_write*sizeof(dirEnt), i*sizeof(dirEnt) );
    }
    ///set the attributes for the content of the new directory
    memset(append_ent[0].dir_name,0x20,11);
    append_ent[0].dir_name[0] = '.';

    /// copy the content of parent dirEnt to '..'
    append_ent[1] = *parent_dirEnt;
    memset(append_ent[1].dir_name,0x20,11);
    append_ent[1].dir_name[0] = '.';
    append_ent[1].dir_name[1] = '.';

    // set the value of the new directory in FAT
    _setFATvalue(new_clus_idx, 0x0FFFFFFF);
    _OS_write_file(new_clus_idx , append_ent, 3*sizeof(dirEnt), 0);

    free(parentdir);
    free(parent_dirEnt);
    return 1;
}

/**
 * @brief
 * @param path
 * @return 1 if the file is created, -1 if the path is invalid, -2 if the final element already exists.
 * The codes are very similar to OS_mkdir,
 * @TODO unify this function with OS_mkdir
 */
int OS_creat(const char *path) {
    if(!_status.initialized) _OS_initialization();

    // first check whether this dir already exists
    dirEnt *self_dirEnt = _OS_getEnt(path);
    // if the file/dir already exists, return with error
    if(self_dirEnt != NULL) {
        free(self_dirEnt);
        return -2;
    }
    // first get the path to the parent dir
    char filename_buffer[12];
    char * parentdir = _get_parent_path(path, filename_buffer);
    if(!parentdir) { // cannot find parent dir
        return -1; // path invalid
    }

    // Then we get the dirEnt of parent dir
    dirEnt * parent_dirEnt = _OS_getEnt(parentdir);

    // if parent dir not found, return with error
    if(parent_dirEnt == NULL) return -1;

    // get the list of parent dir, then we can get how many entries are already used
    dirEnt * list_parent_dirEnt = OS_readDir(parentdir);
    int i = 0;
    while(list_parent_dirEnt[i].dir_name[0]!=0 && list_parent_dirEnt[i].dir_name[0] != 0xE5) i++;
    
    /// if this is the last dirEnt, then write 2 dirEnts (with an empty one)
    /// if this is not the last one (starting with 0xE5), the write one
    int num_ent_write = list_parent_dirEnt[i].dir_name[0] == 0xE5 ? 1 : 2;
    free(list_parent_dirEnt); list_parent_dirEnt = NULL;
    //now i is the pointer to the first empty entries
    

    //get a new cluster
    uint32_t new_clus_idx = _findFirstEmptyClus();
    dirEnt append_ent[2];
    memset(append_ent, 0, 2 * sizeof(dirEnt));

    //set the attributes
    append_ent[0].dir_attr = 0x20; // it is a file
    append_ent[0].dir_wrtTime = 0;
    append_ent[0].dir_wrtDate = 0;
    append_ent[0].dir_fstClusLO = new_clus_idx & 0xFFFF;
    append_ent[0].dir_fstClusHI = new_clus_idx >> 16;
    flnm2FAT(filename_buffer,append_ent[0].dir_name);

    ///write the appended dirEnts to parent dirEnt
    


    if(parent_dirEnt->dir_fstClusLO == 0 && parent_dirEnt->dir_fstClusHI == 0) {
        _OS_write_file(_status.idxRootDirClus , append_ent, num_ent_write*sizeof(dirEnt),
                       i*sizeof(dirEnt));
    } else{
        _OS_write_file(parent_dirEnt->dir_fstClusLO + ((uint32_t)parent_dirEnt->dir_fstClusHI << 16),
                   append_ent, num_ent_write*sizeof(dirEnt), i*sizeof(dirEnt) );
    }
    _setFATvalue(new_clus_idx, 0x0FFFFFFF);
    free(parentdir);
    free(parent_dirEnt);
    return 1;
}

int OS_write(int fildes, const void * buf, int nbytes, int offset) {
    if(!_status.initialized) _OS_initialization();
    // if @fildes is invalid, return -1
    if(_status.openedFiles[fildes] == NULL) {return -1;}

    int actual_written_bytes = _OS_write_file(_status.openedFiles[fildes]->dir_fstClusLO +
        ( (uint32_t)_status.openedFiles[fildes]->dir_fstClusHI << 16 )
        , buf, nbytes, offset);

    // update the file size in the dirEnt
    if (_status.openedFiles[fildes]->dir_fileSize < nbytes + offset ) _status.openedFiles[fildes]->dir_fileSize = nbytes + offset ;

    // write the update of dirEnt to filesystem
    // calculate the idx of start cluster of parent directory
    uint32_t start_idx_parent_dir = _status.openedFilesParent[fildes]->dir_fstClusLO +
                   ( ( (uint32_t)_status.openedFilesParent[fildes]->dir_fstClusHI ) << 16 );

    // if it is the root dir then correct the idx
    if(start_idx_parent_dir == 0) start_idx_parent_dir = _status.idxRootDirClus;
    _update_dirEnt(start_idx_parent_dir, _status.openedFiles[fildes] );
    return actual_written_bytes;
}

///Remove file specified in @path
///@return: 1 if succeed, -1 if the path is valid
///-2 if it is a directory
int OS_rm(const char *path) {
    if(!_status.initialized) _OS_initialization();
    dirEnt *p = _OS_getEnt(path);
    if(p == NULL) return -1;
    int excode = 0;
    uint32_t startClusterIdx;
    switch(p->dir_attr){
        case 0x10:
             excode = -2;
             break;
        case 0x20:
             startClusterIdx = p->dir_fstClusLO + ( ((uint32_t) p->dir_fstClusHI) << 16);
             _remove_link(startClusterIdx);
             char * path_parent = _get_parent_path(path, NULL);
             dirEnt *parentEnt = _OS_getEnt(path_parent);
             if (parentEnt == NULL) excode = -1;
             else {
                 // remove the corresponding entry in parent dir's list
                 _delete_dirEnt(path_parent, p->dir_name);
                 free(path_parent);
             }
             excode = 1;
             break;
        default:
             excode = -1;

    }
    free(p);
    return excode;
}

/** remove the directory specified by @path
* @return : 1 if succeed, -1 if @path is invalid,
* -2 if @path does not refer to a directory
* -3 if @path is not empty
*/
int OS_rmdir(const char *path)
{
    if(!_status.initialized) _OS_initialization();
    int excode = 1;
    // get the dirEnt of this path
    dirEnt * p = _OS_getEnt(path);
    if(p == NULL) {excode = -1; return excode;}
    if(p->dir_attr == 0x10){
        dirEnt * dir_content = OS_readDir(path);
        int i;
        bool isempty = true;
        for( i = 2; dir_content[i].dir_name[0] != '\0'; i++){
            if(dir_content[i].dir_name[0] != 0xE5){ // An empty entry can also start with 0xE5
                isempty = false;
                break;
            }
        }
        free(dir_content);
        if(isempty) {
            printf("Debug: empty file detected\n");
            char * parent = _get_parent_path(path, NULL);
            dirEnt * parent_dirEnt = _OS_getEnt(parent);
            _remove_link( p->dir_fstClusLO + ((uint32_t)p->dir_fstClusHI << 16) );
            printf("Debug: dir body deleted\n");
            _delete_dirEnt(parent, p->dir_name);
            printf("Debug: dir ent deleted\n");
            free(parent_dirEnt);
            free(parent);
        } else {
            excode = -3;
        }
    } else {
        excode = -2;
    }
    free(p);
    return excode;
}
