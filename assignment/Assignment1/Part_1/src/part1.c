/*
Name: Ritesh Kumar
Roll No.: 160575
*/
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<dirent.h>
#include<string.h>

/******************************************************
 * isDirectory:
    * return 1 if mode of path is directory
******************************************************/
int isDirectory(char *path) {
    struct stat pathStruct;
    stat(path, &pathStruct);
    return S_ISDIR(pathStruct.st_mode);
}

/******************************************************
 * len:
    * returns len of string
******************************************************/
int len(char *dir) {
    int count = 0;
    for(int i = 0; dir[i] != 0; i++) count++;
    return count;
}

/******************************************************
 * concat:
    * concats two string and return concated string
******************************************************/
char *concat(char *prevDirPath, char *currDirPath) {
    int prevDirLen = len(prevDirPath), currDirLen = len(currDirPath);
    char *dirPath = (char *)calloc(prevDirLen+currDirLen+2, sizeof(char));
    strcpy(dirPath, prevDirPath);
    dirPath[prevDirLen] = '/';
    dirPath[prevDirLen+1] = 0;
    strcat(dirPath, currDirPath);
    return dirPath;
}

/******************************************************
 * search:
    * searches for search string in the regular file
    * prints the corresponding paragraph
******************************************************/
void search(int fd, char *str, char *path, int isDir) {
    long long buf_size = 1024, cur_size = 0, count = 0;
    char character;
    char *buffer = (char *)calloc(buf_size, sizeof(char));
    while(read(fd, &character, 1) != 0) {
        if(cur_size == buf_size) {
            buf_size *= 2;
            buffer = realloc(buffer, buf_size*sizeof(char)); // dynamically increase size for bigger files
        }
        if(character != '\n') {
            buffer[cur_size] = character;
            cur_size++;
        }
        else {
            buffer[cur_size] = 0;
            if(strstr(buffer, str) != NULL) {
                if(isDir) {
                    printf("%s:%s\n", path, buffer);
                }
                else {
                    printf("%s\n", buffer);
                }
                count++;
            }
            cur_size = 0;
        }
    }
}

/******************************************************
 * searchDirectory:
    * recursively search string in directories
******************************************************/
void searchDirectory(char *str, char *dirPath) {
    // open the current directory
    DIR *dirPtr = opendir(".");
    if(dirPtr == NULL) {
        perror("opendir");
        exit(-1);
    }
    struct dirent *dir;
    while((dir = readdir(dirPtr)) != NULL) {
        if(!strcmp(dir->d_name, "..") || !strcmp(dir->d_name, ".")) continue;
        if(dir->d_type & DT_DIR) {
            // enter the directory
            if(chdir(dir->d_name) == -1) {
                perror("chdir");
                exit(-1);
            }
            // search in the directory
            searchDirectory(str, concat(dirPath, dir->d_name)); // recursive call
            // come back to the directory
            if(chdir("..") == -1) {
                perror("chdir");
                exit(-1);
            }
        }
        else if(dir->d_type & DT_REG) {
            // open the file
            int fd = open(dir->d_name, O_RDONLY);
            // search in the file
            search(fd, str, concat(dirPath, dir->d_name), 1);
            // close the file
            close(fd);
        }
    }
    // close the directory
    closedir(dirPtr);
}

/******************************************************
 * main
******************************************************/
int main(int argc, char *argv[]) {
    if(argc != 3) {
        fprintf(stderr, "Usage: %s <search string> <path>\n", argv[0]);
        exit(-1);
    }
    char *str = argv[1];
    char *path = argv[2];
    if(isDirectory(path)) {
        if(chdir(path) == -1) {
            perror("chdir");
            exit(-1);
        }
        searchDirectory(str, path);
    }
    else {
        int fd = open(path, O_RDONLY);
        search(fd, str, path, 0);
        close(fd);
    }
    return 0;
}