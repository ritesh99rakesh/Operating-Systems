/*
Name: Ritesh Kumar
Roll No.: 160575
*/

/******************************************************
 * Working
  =========
        Parent
        /    \
       /      \
      /        \
Parent <- Pipe <- Child -> findDirSize()
******************************************************/

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<wait.h>
#include<dirent.h>

/******************************************************
 * makePath:
    * extracts last directory from file path
    * eg. IITK/UG -> UG
******************************************************/
char *makePath(char *path) {
    int containSlash = 0;
    long int len = 0, pos;
    for(int i = 0; path[i] != 0; i++) {
        len++;
        if(path[i] == '/') containSlash = 1;
    }
    if(!containSlash) return path;
    for(int i = len-1; i>=0; i--) {
        if(path[i] == '/') {
            pos = i;
            break;
        }
    }
    char *newPath = (char *)calloc((len-pos-1), sizeof(char));
    for(int i = 0; i<len-pos-1; i++) {
        newPath[i] = path[pos+i+1];
    }
    return newPath;
}

/******************************************************
 * finDirSize:
    * recursively travel directories to find their
      size
******************************************************/
long long int findDirSize(char *path) {
    long long int dirSize = 0;
    DIR *dirPtr;
    struct dirent *dir;
    // change to path directory
    if(chdir(path) == -1) {
        perror("chdir");
        exit(1);
    }
    // open directory
    if((dirPtr = opendir(".")) == NULL) {
        perror("opendir");
        exit(1);
    }
    // read directory
    while((dir = readdir(dirPtr)) != NULL) {
        if(!strcmp(dir->d_name, "..") || !strcmp(dir->d_name, ".")) continue;
        // if it is sub-directory call findDirSize
        if(dir->d_type & DT_DIR) {
            dirSize += findDirSize(dir->d_name);
        }
        // if it is regular file, add size
        else if(dir->d_type & DT_REG) {
            struct stat statStruct;
            if(stat(dir->d_name, &statStruct) == -1) {
                perror("stat");
                exit(1);
            }
            dirSize += (long long)statStruct.st_size;
        }
    }
    // close directory
    closedir(dirPtr);
    // change to previous directory
    if(chdir("..") == -1) {
        perror("chdir");
        exit(1);
    }
    return dirSize;
}

/******************************************************
 * main:
    * extracts last directory from file path
******************************************************/
int main(int argc, char *argv[]) {
    if(argc != 2) {
        fprintf(stderr, "Usage: %s <path>\n", argv[0]);
        exit(-1);
    }
    char *path = argv[1];
    DIR *dirPtr;
    struct dirent *dir;
    long long int mainDirSize = 0, dirSize;
    int fd[2], pid;
    // change to path directory
    if(chdir(path) == -1) {
        perror("chdir");
        exit(1);
    }
    // open current directory
    if((dirPtr = opendir(".")) == NULL) {
        perror("opendir");
        exit(1);
    }
    // read current directory
    while((dir = readdir(dirPtr)) != NULL) {
        if(!strcmp(dir->d_name, "..") || !strcmp(dir->d_name, ".")) continue;
        // if it is sub-directory create fork
        if(dir->d_type & DT_DIR) {
            // create pipe to communicate with child
            pipe(fd);
            if((pid = fork()) < 0) {
                perror("fork");
                exit(1);
            }
            else if(pid == 0) {
                // in child part find the size of sub-directory
                dirSize = findDirSize(dir->d_name);
                printf("%s %lld\n", dir->d_name, dirSize);
                close(fd[0]);
                // child writes the sub-directory size to fd
                write(fd[1], &dirSize, sizeof(dirSize));
                close(fd[1]);
                exit(0);
            }
            else {
                // in parent part find add the size of sub-directory
                close(fd[1]);
                // parent reads the sub-directory size from fd
                read(fd[0], &dirSize, sizeof(dirSize));
                close(fd[0]);
                mainDirSize += dirSize;
                // print sub-directory size
            }
        }
        // if it is regular file, add the size of file
        else if(dir->d_type & DT_REG) {
            struct stat statStruct;
            if(stat(dir->d_name, &statStruct) == -1) {
                perror("stat");
                exit(1);
            }
            mainDirSize += (long long)statStruct.st_size;
        }
    }
    // close current directory
    closedir(dirPtr);
    // get new path from path using makePath()
    char *newPath = makePath(path);
    // print root directory size
    printf("%s %lld\n", newPath, mainDirSize);
    return 0;
}