/*
 * Helper functions for reading and writing API for FAT32 filesystem
 * 
 * CS6456 Operating System Homework 4
 * Author: Lu Tian
 * internal helper functions used by implementations:
 * 
 * _OS_initialization(): initialize the environment
 * _OS_getEnt(const char *path): get the dirEnt of file "path"
 * _OS_read_file(unsigned int idxCluster, int offset, int length, void *buffer):
 *   read the content of a file starting at cluster "idxCluster"
 * _OS_write_file(unsigned int idxCluster, const void * buf, int nbytes, int offset):
 *   write the content of buffer to a file start at cluster @idxCluster
 * _setFATvalue(uint32_t idx, uint32_t value): set the value of FAT entry @idx as @value
 * verify(const uint8_t *FATname, char *filename):
 *   Verify that a C string filename equals to a FAT filename
 * flnm2FAT(const char * filename, unsigned char *FATname):
 *   convert filename as C string to FAT dir_name and write it to @FATname
 * _get_parent_path(const char *path, char * filename): split the @path into parent path and 
 *   self name.
 * _findFirstEmptyClus(): find the first available cluster in the file system
 * _remove_link(uint32_t idx): remove the link in FAT starting from @idx
 * _delete_dirEnt(const char *path, const uint8_t name[]): delete the dirEnt with name @name
 */



#include "fat16_32.h"
#include "fat32api.h"
#include "utils32.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/// Verify that a C string filename equals to a FAT filename
int verify(const uint8_t *FATname, char *filename);

int _setFATvalue(uint32_t idx, uint32_t value);

void _initVirtualRootDirEnt(dirEnt * p){
    /// a dirEnt to root dir is a dirEnt with attr 0x10 and 
    /// cluster number 0
    p->dir_attr = 0x10;
    p->dir_fstClusLO=0;
    p->dir_fstClusHI=0;
    return;
}


int _OS_read_file(unsigned int idxCluster, int offset, int length, void * buffer)
{
    int beginLogicCluster = offset / _status.BytesPerCluster;
    int beginOffset = offset % _status.BytesPerCluster;
    int endLogicCluster = (offset + length + _status.BytesPerCluster - 1) / _status.BytesPerCluster;
    int i = 0;
    uint32_t * tmpbuffer = malloc(_status.BytesPerSec);
    unsigned char * tmpDataBuffer = malloc(_status.BytesPerCluster);
    int readCnt = 0;
    int err_code = 0; // output code.
    unsigned int tmpPhysicalCluster = idxCluster;
    while(i < endLogicCluster){
        //puts("reading");
        // Check whether tmpPhysicalCluster is valid;
        if(tmpPhysicalCluster == 0xFFFF) { // if not valid
            err_code = -1; // output 1
            break;
        }
        // if we should read, read the content to buffer;
        if(i >= beginLogicCluster){
            lseek(_status.device_fd, _status.startDataSec * _status.BytesPerSec
 + (tmpPhysicalCluster - 2) * _status.BytesPerCluster, SEEK_SET);
            read(_status.device_fd, tmpDataBuffer, _status.BytesPerCluster);// read the current cluster to tmp buffer
            if(i == beginLogicCluster){// if this is the begin cluster
                if(endLogicCluster - beginLogicCluster == 1){ // if within one cluster
                    memcpy(buffer+readCnt, tmpDataBuffer + beginOffset, length);
                    readCnt += length;
                } else { // if this is not the last cluster, copy all data to the end
                    memcpy(buffer+readCnt, tmpDataBuffer + beginOffset,
                        _status.BytesPerCluster - beginOffset);
                    readCnt += _status.BytesPerCluster - beginOffset;
                }
            } else { // if this is not the begin cluster
                if(endLogicCluster - i == 1){ // if this is the last cluster
                    memcpy(buffer+readCnt, tmpDataBuffer, length - readCnt);
                    readCnt = length;
                } else {
                    memcpy(buffer+readCnt, tmpDataBuffer, _status.BytesPerCluster);
                    readCnt += _status.BytesPerCluster;
                }
            }
        }
        if(readCnt == length) {err_code = readCnt; break;} // if all data are read, break

        // update tmpPhysicalCluster
        // calc where it is in FAT (in sector)
        unsigned int posFATsec = _status.startFATSec +
            (tmpPhysicalCluster * SIZE_FAT_ENTRY) / _status.BytesPerSec;
        int posFAToffset = ((tmpPhysicalCluster * SIZE_FAT_ENTRY) % _status.BytesPerSec) / SIZE_FAT_ENTRY;
        lseek(_status.device_fd, posFATsec * _status.BytesPerSec, SEEK_SET);
        read(_status.device_fd,tmpbuffer,_status.BytesPerSec);
        tmpPhysicalCluster = tmpbuffer[posFAToffset] & 0x0FFFFFFF; // get next cluster number
        i++;
    }
    free(tmpbuffer);
    free(tmpDataBuffer);
    return err_code;
}

/**
verify if a FAT name is the same with a filename in C string
@FATname: string with 11 chars, not 0
@filename: string with 0
if the same, return 0, otherwise 1.
*/
int verify(const uint8_t *FATname, char *filename) {
    int fn_length = strlen(filename),posPoint, lengthExt;
    if(fn_length > 12) return 1; // the totoal filename should be <= 12 chars
    if(filename[0]=='.'){
        if(filename[1]=='\0') {
            posPoint=1;
            lengthExt=0;
        } else if(filename[1]=='.' && filename[2]=='\0') {
            posPoint=2;
            lengthExt=0;
        } else{
            return 1;
        }
    } else {
        posPoint = strcspn(filename, "."); // the position of '.' in filename

        if(posPoint > 8) return 1; // the main name should be <= 8 chars
        lengthExt =  fn_length - posPoint - 1; // length of extension
        if(lengthExt < 0) lengthExt = 0; //if no dot, correct the lengthExt
        if(lengthExt> 3) return 1; // extension should be <= 3 chars
    }
    int i;
    for(i = 0 ; i < 11; i ++)
    {
        if(i < posPoint && FATname[i] != toupper(filename[i]) ) return 1;
        if(i >= posPoint && i < 8  && FATname[i]!=0x20) return 1;
        if(i >= 8 && (i - 8 < lengthExt) && FATname[i] != toupper(filename[i - 8 + posPoint + 1] ))
            return 1;
        if(i >= 8 && (i - 8 >= lengthExt) && FATname[i] != 0x20) return 1;
    }
    return 0;
}

/**
 * @brief convert filename as C string to FAT dir_name and write it to @FATname
 * @param filename: C string filename
 * @param FATname: filename in FAT dirEnt
 * @return 0 if succeed, 1 if failure
 */
int flnm2FAT(const char * filename, unsigned char *FATname) {
    int j=0; // reading pointer
    int ptr_w = 0; // writing pointer
    while(filename[j]!='\0'){
        if(filename[j]=='.'){
            if(ptr_w>8) return 1;
            for( ; ptr_w < 8; ptr_w++) FATname[ptr_w]=0x20; // complete the main filename
        } else {
            if(ptr_w>11) return 1;
            FATname[ptr_w] = toupper(filename[j]);
            ptr_w++;
        }
        j++;
    }
    for( ; ptr_w < 11; ptr_w++) FATname[ptr_w]=0x20; // complete the main and extended filename
    return 0;
}


/// get the dirEnt of the specified file/dir
dirEnt * _OS_getEnt(const char * path){
    const char *p = path;
    dirEnt tmpdir = *_status.curdir; /*tmp dir in searching*/
    dirEnt * buf = malloc(_status.BytesPerCluster); /*temporary buffer*/
    if (path[0]=='/'){
        _initVirtualRootDirEnt( &tmpdir );
        /* absolute path */
        p++;// pointer p always points to the filename that we are searching for
    }
    int curFileNameLength = 0; // store filename length detected in path
    bool flagFound = true;// the flag indicating that we found the file
    while(curFileNameLength = strcspn(p,"/"), curFileNameLength != 0){
        //make an individual copy of the file name
        if(curFileNameLength == 1 && p[0]=='.') { // if current filename is a single '.'
            p++; // ignore this filename
            if(*p == '/') p++;
            continue; //deal with the remaining
        }
        flagFound = false;
        char *fileName = malloc(sizeof(char) * curFileNameLength + 1);
        memcpy(fileName, p, curFileNameLength);
        fileName[curFileNameLength] ='\0';
        if(tmpdir.dir_fstClusLO == 0 && tmpdir.dir_fstClusHI == 0){
            /* tmp dir is root*/
            if(fileName[0] == '.' && fileName[1]=='.' && fileName[2]=='\0') {
                // if the filename is '.' or '..'
                flagFound=true;
            }
            else {

                //lseek(_status.device_fd,_status.startRootDirSec * _status.BytesPerSec, SEEK_SET);
                //go over the whole root directory
                int i = 0;
                bool endOfDirectory = false;
                for( ; true ; i++) {
                    _OS_read_file(_status.idxRootDirClus, i * _status.BytesPerCluster, _status.BytesPerCluster, buf);
                    int j = 0;
                    for( ; j < _status.BytesPerCluster / sizeof(dirEnt); j++){
                        if(buf[j].dir_name[0]=='\0') {
                            endOfDirectory = true;
                            break;
                        }
                        if( !verify(buf[j].dir_name, fileName)) {
                            flagFound = true;
                            tmpdir = buf[j];
                            break;
                        }
                    }
                if(endOfDirectory) break;
                if(flagFound) break;
                }
            }
        }else{
            /* tmp dir is not root*/
            // if tmpdir is not a dir, throw error
            if(!(tmpdir.dir_attr & 0x10)){
                free(fileName);
                free(buf);
                return NULL;
            }
            // get the starting cluster idx of the current dir
            unsigned int fstCluster = tmpdir.dir_fstClusLO + tmpdir.dir_fstClusHI * 65536;
            //printf("fstCluster is %x\n", fstCluster);
            int i = 0;
            bool endOfDirectory=0;
            for( ; true ; i++){
                _OS_read_file(fstCluster, i * _status.BytesPerCluster, _status.BytesPerCluster, buf);
                int j = 0;

                for( ; j < _status.BytesPerCluster / sizeof(dirEnt); j++){
                    if(buf[j].dir_name[0]=='\0') {
                        endOfDirectory = true;
                        break;
                    }
                    if( !verify(buf[j].dir_name, fileName)) {

                        flagFound = true;
                        tmpdir = buf[j];
                        break;
                    }
                }
                if(endOfDirectory) break;
                if(flagFound) break;
            }
        }
        p += (curFileNameLength); // update pointer to the next dir name
        if(p[0] == '/' ) p++;
        free(fileName);
    }
    free(buf);
    if(flagFound){
        dirEnt * output = malloc(sizeof(dirEnt));
        *output = tmpdir;
        return output;
    } else {
        return NULL;
    }
}

char * _get_parent_path(const char *path, char * filename) {
    size_t lenPath = strlen(path);
    if(lenPath==0) return NULL; // if no path, then cannot extract parent path
    if(lenPath==1 && path[0] == '/') return NULL; //if it is a single /, return NULL
    const char *p = path + lenPath - 1;// pointer to the last char of the path
    if(*p == '/') p--;// if the last char is a /, ignore it
    int filename_length = 0;

    /// go over the path backward
    while(p >= path && p[0]!='/') {p--; filename_length++;}
    if(filename) {
        strncpy(filename, p+1, filename_length);
        filename[filename_length] = '\0';
    }
    if(p < path) { // if '/' is not found, then parent dir should be CWD. Return an empty string
        char *res = malloc(sizeof(char));
        res[0] = '\0';
        return res;
    } else { // if found '/'
        size_t lenParentPath = p - path + 1;
        char *res = malloc((lenParentPath + 1) * sizeof(char) );
        strncpy(res,path,lenParentPath);
        res[lenParentPath] = '\0';
        return res;
    }
}


int _OS_write_file(unsigned int idxCluster, const void * buf, int nbytes, int offset)
{
    unsigned int beginLogicCluster = offset / _status.BytesPerCluster;
    unsigned int beginOffset = offset % _status.BytesPerCluster;
    unsigned int endLogicCluster = (offset + nbytes + _status.BytesPerCluster - 1) / _status.BytesPerCluster;
    uint32_t * tmpFATbuffer = malloc(_status.BytesPerSec); // buffer storing FAT entries
    unsigned char * tmpDataBuffer = malloc(_status.BytesPerCluster); // buffer storing data
    unsigned int i = 0; // current logic cluster number
    int writeCnt = 0; // how many bytes are written
    int err_code = 0; // output code.
    unsigned int tmpPhysicalCluster = idxCluster;
    while(i < endLogicCluster){

        // if we should write, write the content to buffer;
        if(i >= beginLogicCluster){
            lseek(_status.device_fd, _status.startDataSec * _status.BytesPerSec
 + (tmpPhysicalCluster - 2) * _status.BytesPerCluster, SEEK_SET);

            // read the current cluster to buffer
            read(_status.device_fd, tmpDataBuffer, _status.BytesPerCluster);// read the current cluster to tmp buffer

            // relocate the reading/writing pointer
            lseek(_status.device_fd, _status.startDataSec * _status.BytesPerSec
 + (tmpPhysicalCluster - 2) * _status.BytesPerCluster, SEEK_SET);
            if(i == beginLogicCluster){// if this is the begin cluster
                if(endLogicCluster - beginLogicCluster == 1){ // if within one cluster
                    memcpy(tmpDataBuffer + beginOffset, buf+writeCnt,  nbytes);
                    writeCnt += nbytes;

                } else { // if this is not the last cluster, copy all data to the end
                    memcpy(tmpDataBuffer + beginOffset, buf+writeCnt,
                        _status.BytesPerCluster - beginOffset);
                    writeCnt += _status.BytesPerCluster - beginOffset;
                }
            } else { // if this is not the begin cluster
                if(endLogicCluster - i == 1){ // if this is the last cluster
                    memcpy(tmpDataBuffer, buf + writeCnt,  nbytes - writeCnt);
                    writeCnt = nbytes;
                } else {
                    memcpy(tmpDataBuffer, buf + writeCnt,  _status.BytesPerCluster);
                    writeCnt += _status.BytesPerCluster;
                }
            }

            //write the buffer to disk
            write(_status.device_fd, tmpDataBuffer, _status.BytesPerCluster);
        }
        if(writeCnt == nbytes) {err_code = writeCnt; break;} // if all data are read, break

        // update tmpPhysicalCluster
        // calc where it is in FAT (in sector)
        unsigned int posFATsec = _status.startFATSec +
            (tmpPhysicalCluster * SIZE_FAT_ENTRY) / _status.BytesPerSec;

        // calculate the offset in this sector
        int posFAToffset = (tmpPhysicalCluster * SIZE_FAT_ENTRY) % _status.BytesPerSec / SIZE_FAT_ENTRY;

        // read this sector
        lseek(_status.device_fd, posFATsec * _status.BytesPerSec, SEEK_SET);
        read(_status.device_fd,tmpFATbuffer,_status.BytesPerSec);

         // get next cluster number
        uint32_t newPhysicaCluster = ( tmpFATbuffer[posFAToffset] & 0x0FFFFFFF );

        // if reach the end of the file, try to allocate more space
        if(newPhysicaCluster == 0x0FFFFFFF) {
            newPhysicaCluster = _findFirstEmptyClus();

            // if cannot allocate cluster, abort
            if(newPhysicaCluster == 0) {
                /*TODO: abort */
                break;
            } else {
                // save this newly allocated cluster to FAT
                _setFATvalue(tmpPhysicalCluster, newPhysicaCluster);
                _setFATvalue(newPhysicaCluster, 0xFFFFFFFF);
                tmpPhysicalCluster = newPhysicaCluster;
            }
        } else {
            // if not the end of file, just update tmpPhysicalCluster
            tmpPhysicalCluster =  newPhysicaCluster;
        }
        i++;
    }
    free(tmpFATbuffer);
    free(tmpDataBuffer);
    return err_code;
}

/*
 * Find the first empty cluster in FAT
 * return cluster (>=2) index if succeed
 * return 0 if not succeed
 * */
unsigned int _findFirstEmptyClus() {
    uint32_t * buf = malloc(_status.BytesPerSec);
    unsigned int i = 0;
    lseek(_status.device_fd, _status.BytesPerSec * _status.startFATSec, SEEK_SET);
    unsigned int result = 0;
    for( ; i < _status.FATSz; i++) { // go over all FAT entries
        read(_status.device_fd, buf, _status.BytesPerSec);
        int j = 0;
        for( ; j < _status.BytesPerSec / SIZE_FAT_ENTRY; j++) {
            if(buf[j] == 0) {
                result = i * (_status.BytesPerSec / SIZE_FAT_ENTRY) + j;
                break;
            }
        }
        if(result != 0) break;
    }

    free(buf);
    return result;
}


/**
 * set the FAT entry @idx to value @value
 * */
int _setFATvalue(uint32_t idx, uint32_t value) {
    unsigned int posFATsec = _status.startFATSec +
        (idx * SIZE_FAT_ENTRY) / _status.BytesPerSec;
    int posFAToffset = ((idx * SIZE_FAT_ENTRY) % _status.BytesPerSec) / SIZE_FAT_ENTRY;
    int i;
    uint32_t * buf = malloc(_status.BytesPerSec);
    if(buf == NULL) return 1;

    // change all FATs
    for(i = 0; i < _status.numFAT; i++){

        lseek(_status.device_fd, posFATsec * _status.BytesPerSec, SEEK_SET);
        read(_status.device_fd, buf, _status.BytesPerSec);
        uint32_t tmp = buf[posFAToffset] & 0xF0000000;
        buf[posFAToffset] = tmp + (value & 0x0FFFFFFF);

        lseek(_status.device_fd, posFATsec * _status.BytesPerSec, SEEK_SET);
        write(_status.device_fd, buf, _status.BytesPerSec);
        posFATsec += _status.FATSz;
    }
    free(buf);
    return 0;
}

/// remove the link in FAT starting from @idx
int _remove_link(uint32_t idx) {
    uint32_t * buf = malloc(_status.BytesPerSec);
    uint32_t curidx = idx;
    while( (curidx & 0x0FFFFFFF) != 0x0FFFFFFF && curidx != 0) {
        uint32_t posFATsec = _status.startFATSec +
            (curidx * SIZE_FAT_ENTRY) / _status.BytesPerSec;
        int posFAToffset = ((idx * SIZE_FAT_ENTRY) % _status.BytesPerSec) / SIZE_FAT_ENTRY;
        lseek(_status.device_fd, posFATsec * _status.BytesPerSec, SEEK_SET);
        read(_status.device_fd, buf, _status.BytesPerSec);
        uint32_t newidx = buf[posFAToffset];
        _setFATvalue(curidx,0);
        curidx = newidx & 0x0FFFFFFF;
        /*buf[posFAToffset] = 0;
        lseek(_status.device_fd, posFATsec * _status.BytesPerSec, SEEK_SET);
        write(_status.device_fd, buf, _status.BytesPerSec);*/
    }
    free(buf);
    return 0;
}

/**
 * update the dirEnt in list starting from @idxCluster
 * update the entry with name @ptr->dir_name to the content pointed by @ptr
 * @return 0 if success, 1 if failure
 */
int _update_dirEnt(uint32_t idxCluster, dirEnt * ptr) {
    // find the location of this dirEnt
    bool found = false;
    dirEnt * buf = malloc(_status.BytesPerCluster);
    int i = 0;
    for( ; !found; i++) {
        _OS_read_file(idxCluster, i * _status.BytesPerCluster, _status.BytesPerCluster, buf);
        int j = 0;
        bool reach_end = false;
        for( ; j < _status.BytesPerCluster / sizeof(dirEnt); j++) {
            if(buf[j].dir_name[0] == '\0') {
                reach_end = true;
                break;
            }

            // if we found the corresponding dirEnt, update it and write it to the disk
            if(strncmp((const char *)buf[j].dir_name, (const char *) ptr->dir_name, 11 ) == 0) {
                found = true;
                _OS_write_file(idxCluster, ptr, sizeof(dirEnt), i * _status.BytesPerCluster + j * sizeof(dirEnt));
                break;
            }
        }
        if(reach_end)
            break;
    }

    free(buf);
    return found ? 0 : 1;
}

/**
delete the entry with name @name from the list of dir @path
return 0 if succeed, 1 if fail
*/
int _delete_dirEnt(const char *path, const uint8_t name[]) {
    dirEnt * my_dirEnt = _OS_getEnt(path);
    uint32_t start_idx = my_dirEnt->dir_fstClusLO + ( (uint32_t)my_dirEnt->dir_fstClusHI << 16);

    // if this path is the root dir, then we set @start_idx correctly
    if(start_idx==0) start_idx = _status.idxRootDirClus;

    dirEnt * dir_content = OS_readDir(path);
    int i = 0;
    bool found = false;
    for( ; dir_content[i].dir_name[0]!= '\0'; i++)
    {
        if(strncmp((const char *)dir_content[i].dir_name, (const char *)name, 11) == 0)
        {
            dir_content[i].dir_name[0] = 0xE5;
            _OS_write_file(start_idx, & dir_content[i], sizeof(dirEnt), i * sizeof(dirEnt));
            found = true;
            break;
        }
    }
    free(dir_content);
    free(my_dirEnt);
    return found ? 0 : 1;
}
