#include <stdio.h>
#include "fat16_32.h"
#include "fat32api.h"
#include "utils32.h"
#include <string.h>
#include <stdlib.h>


int main()
{
    _OS_initialization();
    unsigned errcode1 = OS_mkdir("/mydir");
    printf("%d\n", errcode1);
    unsigned errcode2 = OS_mkdir("/mydir/mydir2");
    printf("%d\n", errcode2);
    return 0;
}
/*
{
    _OS_initialization();
    dirEnt * ent = _OS_getEnt("/people/bhs2e/jokes2.txt");
    char s[]="TEST STRING";
    char buffer[20];
    uint32_t offset = 4096;
    _OS_read_file(ent->dir_fstClusLO + ((uint32_t)ent->dir_fstClusHI << 16), offset, 11, buffer);
    buffer[11]='\0';
    puts(buffer);
    _OS_write_file(ent->dir_fstClusLO + ((uint32_t)ent->dir_fstClusHI << 16), s, 11, offset);
    _OS_read_file(ent->dir_fstClusLO + ((uint32_t)ent->dir_fstClusHI << 16), offset, 11, buffer);
    buffer[11] = '\0';
    puts(buffer);
    free(ent);
    return 0;
}
*/



void moveto(const char * path)
{
    if(OS_cd(path) == 1)
        printf("moved to '%s'\n", path);
    else
        printf("FAILED: Cannot move to '%s'\n", path);
}


void ls(const char * path)
{
    char buf[12] = "";
    dirEnt * dir = OS_readDir(path);
    if (dir)
    {
        strncpy(buf, (char*)dir->dir_name, 11);
        printf("\nDirectory '%s':\n", buf);
    }
    else
    {
        printf("path does not exist\n");
        return;
    }
    dirEnt *p = dir;
    while(p && p->dir_name[0])
    {
        strncpy(buf, (char *)p->dir_name, 11);
        buf[11] = '\0';
        printf("%s\n", buf);
        p++;
    }
    free(dir);
    printf("\n");
    return;
}

int myopen(const char * file)
{
    int fildes = OS_open(file);
    printf("Opening file '%s' returned: %d\n", file, fildes);
    return fildes;
}

void myclose(int fd)
{
    printf("closing fildes %d returned: %d\n", fd, OS_close(fd));
}

int myread(int fildes, void * buf, int nbyte, int offset)
{
    int result = OS_read(fildes, buf, nbyte, offset);
    printf("reading fd %d gives return of %d\n", fildes, result);
    return result;
}


/*

int main()
{


    char file[] = "the-game.txt";
    char file1[] = "../people/crj8j/repeat.txt";

    myclose(10);
    moveto("people");
    moveto("yyz5w");

    ls("../../people/");


    char buf[20] = "";
    int fildes = myopen(file);
    myread(100, buf, 19, 0);
    int read_size = OS_read(fildes, buf, 19, 0);

    int total_size = 0;
    while (read_size > 0)
    {
        buf[read_size] = '\0';
        printf("%s", buf);
        total_size += read_size;
        read_size = OS_read(fildes, buf, 19, total_size);
        //printf("%d\n",read_size);
    }



    moveto("/");
    moveto("people/ajw3m");
    ls(".");
    moveto("..");
    ls(".");
    ls("..");
    moveto("..");
    ls(".");
    ls("..");
    ls("/");
    printf("Open file '%s' is %d\n", file, OS_open(file));
    moveto("ajw3m");
    printf("Open file '%s' is %d\n", file, OS_open(file));
    printf("Open file '%s' is %d\n", file1, OS_open(file1));
	myclose(1);
	myclose(1);
	myclose(2);

    return 0;
}
*/