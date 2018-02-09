#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <dlfcn.h>

#include "myfat.h"

void readNonEmptyLine(char *line);
int validLine(char* line);
unsigned char *readFile(int fd, int numbytes, int offset);
unsigned char *copyFile(char *name, int numbytes, int offset);
//char *convertName(char *name);
char *trimwhitespace(char *str);
int tokenize(char *line, char **args, char *argDelim);

char *map[128];


int main(int argc, char *argv[]) {
	//int i = 0;
	char line[1024];
	char argDelim[2] = " ";
	//string pipeDelim = string("|");

	while(1) {
		
		// read input
		printf("#");
		fflush(stdout);
		readNonEmptyLine(line);
		printf("%s\n", line);
		fflush(stdout); 
		if (strcmp(line, "exit") == 0){ 
			//printf("\n");
			break;
		}

		// check if the line is valid or not
		if (!validLine(line)) {
			printf("ERROR: invalid input\n");
			fflush(stdout);
			continue;
		}

			
		// start processing the commands
		//int commandNo = 0;
		
		int i = 0;
		int argCount = 0;
		char *args[15];
		argCount = tokenize(line, args, argDelim) ;
		//printf("argsCount = %d \n", argCount);
		
		if(strcmp(args[0], "cd") == 0){
			printf("changing directory to %s\n", args[1]);
			//printf("Before CD: %s\n", cwdPath);
			//int status = 0;
			int status = OS_cd(args[1]);
			if(status == 1) 
				printf("Successful\n");
			else 
				printf("Not Successful\n");   
				// printf("After CD: %s\n", cwdPath);
			fflush(stdout);
		}
		else if(strcmp(args[0], "cp") == 0){
			printf("copying file %s of size %d to %s \n", args[1], atoi(args[3]), args[2]);
			//unsigned char *readFile(char *name, int numbytes, int offset);
			unsigned char *readbuf = copyFile(args[1], atoi(args[3]), 0);
			//unsigned char *readbuf = "";
			FILE *write_ptr;
			write_ptr = fopen(args[2],"wb"); 
			fwrite(readbuf,atoi(args[3]),1,write_ptr);
			fclose(write_ptr);
			// printf("After CD: %s\n", cwdPath);
			fflush(stdout);
		}   
		else if(strcmp(args[0], "read") == 0){
			if(argCount >= 4) {
				for(i = 0; i < 128; i++){
					if(strcmp(map[i], args[1]) == 0){
					    break;
					}    
				}
				unsigned char *readout = readFile(i, atoi(args[2]), atoi(args[3]));
				//unsigned char *readout = NULL;
				if(readout != NULL)
					printf("\n%s\n", readout);
				       
			} 
			else printf("ERROR: not enough argument for read");
			fflush(stdout);
		}
		else if(strcmp(args[0], "open") == 0){
			printf("Opening file %s\n", args[1]);
			//printf("Before open: %s\n", cwdPath);
			//int fd = -1;
			int fd = OS_open(args[1]);
			if(fd != -1){ 
				printf("File opened successfully with file descriptor %d\n", fd);
				map[fd] = (char *) malloc (12 * sizeof(char));
				strcpy(map[fd], args[1]);
			}
			else printf("File open error \n");
			fflush(stdout);
			//printf("After CD: %s\n", cwdPath);
		}
		else if(strcmp(args[0], "close") == 0){
			printf("close file %s\n", args[1]);
			//printf("Before open: %s\n", cwdPath);
			//int status = -1;
			int status = OS_close(atoi(args[1]));
			printf("status: %d\n", status);
			fflush(stdout);
			//printf("After CD: %s\n", cwdPath);
		}      
		else if(strcmp(args[0], "lsdir") == 0){
			char *path = (char *) malloc(100 * sizeof(char));
			
			if(argCount < 2){
				strcpy(path, ".");
				//printf("listing directory %s\n", cwdPath);
			}
			else{
				strcpy(path, args[1]);
			}
			printf("listing directory %s\n", path);
			dirEnt *dirs = OS_readDir(path);
			if(dirs != NULL){
			//printf("%12s %4s %4s\n", "File Name", "Attr", "Size");
				int t = 0;
				for(i=0; i<1000; i++) {
					if(dirs[i].dir_name[0] == 0) {
						printf("listed %d entries\n", t);
						break;
					}
					else if(dirs[i].dir_attr == 0x10 || dirs[i].dir_attr == 0x20 || dirs[i].dir_attr == 0){
					  if(dirs[i].dir_name[0] == 0xE5) continue;
					  char dir_name[12];
					  dir_name[11] = 0;
					  int j = 0;
					  for(j = 0; j < 11; j++){ 
					    if(dirs[i].dir_name[j] == ' '){
					      dir_name[j] = '\0';
					      break;
					    } 
					    dir_name[j] = dirs[i].dir_name[j];
					  }
	      
					  printf("%12s %04x %4ld\n", dir_name, dirs[i].dir_attr, (long) dirs[i].dir_fileSize);
					  t++;
					}
				}
			}
			else{
				printf("ERROR: %s is not a directory\n", path);
			}
			fflush(stdout);
		} 
		else{
			printf("ERROR: invalid command\n");
		}
	}
	
	return 0;
}

int tokenize(char *line, char **args, char *argDelim){
	char *token;
	int i = 0;
	token = strtok(line,argDelim);
	
	while( token != NULL ){
		//printf("%d\n", strlen(token));
		args[i] = (char *) malloc (strlen(token) * sizeof(char));
		strcpy(args[i], token);
		i++; 
		token = strtok(NULL,argDelim);
	}
	return i;
}



char *trimwhitespace(char *str){
  char *end;

  // Trim leading space
  while(isspace((unsigned char)*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace((unsigned char)*end)) end--;

  // Write new null terminator
  *(end+1) = 0;

  return str;
}

void readNonEmptyLine(char *line) {
        while (1) {
                fgets(line, 1024, stdin);
                trimwhitespace(line);
                if (strlen(line) > 0) break;
        }
}

int validLine(char *chArray) {
	int i = 0;
	int length = strlen(chArray);
	for (i = 0; i < length; i++) {
		char ch = chArray[i];
		if (!((ch >= 'A' && ch <= 'Z') || ( ch >= 'a' && ch <= 'z') 		// letters
				|| (ch >= '0' && ch <= '9') 				// numbers
				|| ch == '.' || ch == '-' || ch == '_' || ch == '/'	// special characters
				|| ch == ' ' || ch == '\t'				// white spaces 
				|| ch == '<' || ch == '>' 
				|| ch == '~')) {		// operators
			return 0;
		}
	}
	if (chArray[length - 1] == '|') return 0;					// meaningless pipe at the end
	return 1;
}


unsigned char *readFile(int fd, int numbytes, int offset){
  
  if(fd == -1){
    printf("ERROR: Invalid FileName\n");
    return NULL;
  }
  unsigned char *buf = (unsigned char *)malloc(numbytes*sizeof(unsigned char));
  //buf[0] = '\0';
  int bytesRead = OS_read(fd, buf, numbytes, offset);
  printf("Reading %d bytes from file %d starting from offset %d\n", bytesRead, fd, offset);
  return buf;
}


unsigned char *copyFile(char *name, int numbytes, int offset){
  int fd = OS_open(name);
  if(fd == -1){
    printf("ERROR: Invalid FileName\n");
    return NULL;
  }
  unsigned char *buf = (unsigned char *)malloc(numbytes*sizeof(unsigned char));
  //buf[0] = '\0';
  int bytesRead = OS_read(fd, buf, numbytes, offset);
  printf("Reading %d bytes from file %s starting from offset %d\n", bytesRead, name, offset);
  return buf;
}


