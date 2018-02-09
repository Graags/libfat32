/**
 Header file for the helper functions used by FAT32 API implementation
 Author: Lu Tian
 */

#ifndef _UTILS_H
#define _UTILS_H

#include "fat16_32.h"
#include "fat32api.h"


/// convert a dirEnt @p into a dirEnt for the root directory
void _initVirtualRootDirEnt(dirEnt * p);

/// get the dirEnt of the file/dir corresponding to @path
/// it is the user's responsibility to free the pointer
dirEnt * _OS_getEnt(const char * path);

/** read a file starting at idxCluster
 idxCluster: the starting cluster of the file
 offset: where reading begins (in bytes)
 length: how long is the buffer (in bytes)
 buffer: where the content is stored
 */
int _OS_read_file(unsigned int idxCluster, int offset, int length, void * buffer);

/** get the path of the parent dir of this file/dir
 return a pointer to a newly allocated string if succeed. Users are responsible to free it.
 if not succeed, return NULL
 if @filename is not NULL, then the filename is written there
 users are responsible to allocate enough memory for @filename
 if @filename is NULL, then we ignore it */
char * _get_parent_path(const char *path, char * filename);

/**
 write the file specified by the starting cluster
 @idxCluster: index of starting cluster of the file
 @buf: pointer to memory block
 @nbytes: bytes to be written
 @offset: where to start writing */
int _OS_write_file(unsigned int idxCluster, const void * buf, int nbytes, int offset);


int flnm2FAT(const char * filename, unsigned char *FATname);

unsigned int _findFirstEmptyClus();

int _setFATvalue(uint32_t idx, uint32_t value);


/**
 * update the dirEnt in list starting from @idxCluster
 * update the entry with name @ptr->dir_name to the content pointed by @ptr
 * @return 0 if success, 1 if failure
 */
int _update_dirEnt(uint32_t idxCluster, dirEnt * ptr);

/// remove the link in FAT starting from @idx
int _remove_link(uint32_t idx);

/**
delete the entry with name @name from the list of dir @path
return 0 if succeed, 1 if fail
*/
int _delete_dirEnt(const char *path, const uint8_t name[]);
#endif
