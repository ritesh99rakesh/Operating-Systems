/*
Name: Ritesh Kumar
Roll No.: 160575
*/
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<wait.h>
#include<fcntl.h>

/******************************************************
 * formCommand:
    * forms the command for $ operator
******************************************************/
char **formCommand(int argc, char **argv) {
    char **command = (char **)malloc((argc-5+1)*sizeof(char *));
    for(int i = 5; i<argc; i++) command[i-5] = argv[i];
    command[argc-5] = (char *)NULL;
    return command;
}

/******************************************************
 * main:
    * @ operator
        * creates a pipe
        * in the child runs the `grep` command with
          pipe as stdout
        * in the parent runs the `wc -l` command with
          pipe as stdin
    
    * $ operator
        * creates a pipe
        * parent
          /   \
      parent  child1
              /   \
          child1a child1b

        * in child1a run `grep` command with pipe as
          stdout
        * in child1b run `grep` command with output
          file as stdout
        * in parent run `command` with pipe at stdin
******************************************************/
int main(int argc, char *argv[]) {
    if(argc < 2) {
        fprintf(stderr, "Usage: %s <operator> <args>\n", argv[0]);
        exit(-1);
    }
    char operator = argv[1][0], *str, *path;
    if(operator == '@') {
        if(argc != 4) {
            fprintf(stderr, "Usage: %s <operator> <search string> <path>\n", argv[0]);
            exit(-1);    
        }
        str = argv[2], path = argv[3];
        int pipefd[2], pid;
        if(pipe(pipefd) == -1) {
            perror("pipe");
            exit(1);
        }
        // pipefd[0] -> reading; pipefd[1] -> writing;
        if((pid = fork()) < 0) {
            perror("fork");
            exit(-1);
        }
        // child process
        if(!pid) {
            dup2(pipefd[1], 1);
            close(pipefd[0]);
            execlp("grep", "grep", "-rF", str, path, NULL);
            perror("execlp");
        }
        // parent process
        else {
            wait(NULL);
            dup2(pipefd[0], 0);
            close(pipefd[1]);
            execlp("wc", "wc", "-l", NULL);
            perror("execlp");
            exit(1);
        }
    }
    else if(operator == '$') {
        if(argc < 6) {
            fprintf(stderr, "Usage: %s <operator> <search string> <path> <output file> <command>\n", argv[0]);
            exit(-1);    
        }
        str = argv[2], path = argv[3];
        char *output = argv[4];
        char **command = formCommand(argc, argv);
        int pipefd[2], pid;
        if(pipe(pipefd) == -1) {
            perror("pipe");
            exit(1);
        }
        if((pid = fork()) < 0) {
            perror("fork");
            exit(1);
        }
        else if(pid == 0) {
            // child1 process
            if((pid = fork()) < 0) {
                perror("fork");
                exit(1);
            }
            else if(pid == 0) {
                // child1a process
                int fd = open(output, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
                dup2(fd, 1);
                execlp("grep", "grep", "-rF", str, path, NULL);
                perror("execlp");
                close(fd);
                exit(1);
            }
            else {
                // child1b process
                dup2(pipefd[1], 1);
                close(pipefd[0]);
                execlp("grep", "grep", "-rF", str, path, NULL);
                perror("execlp");
                exit(1);
            }
        }
        else {
            // parent process
            wait(NULL);
            dup2(pipefd[0], 0);
            close(pipefd[1]);
            execvp(command[0], command);
            perror("execve");
            exit(1);
        }
    }
    return 0;    
}