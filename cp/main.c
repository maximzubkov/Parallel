#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pwd.h>
#include <time.h>
#include <grp.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

int main(int argc, char * argv[]){
    printf("source: %s, dist: %s\n", argv[1], argv[2]);
    int fdin, fdout;
    struct stat src_statbuf;
    if ((fdin = open(argv[1], O_RDONLY)) < 0){
        perror("source problem");
        return 0;
    }
    if (fstat(fdin, &src_statbuf) < 0){
        perror("source stat problem");
        return 0;
    }
    if(S_ISDIR(src_statbuf.st_mode)){
        perror("source is a directory");
        return 0;
    }
    if (opendir(argv[2]) != NULL){
        char path[strlen(argv[2]) + strlen(argv[1]) + 1];
        memcpy(path, argv[2], strlen(argv[2]));
        strcat(path, "/");
        strcat(path, argv[1]);
        if ((fdout = open(path, O_RDWR | O_CREAT | O_TRUNC)) < 0) {
            perror("destination problem");
            return 0 ;
        }
    } else {
        if ((fdout = open(argv[2], O_RDWR | O_CREAT | O_TRUNC)) < 0) {
            perror("destination problem");
            return 0 ;
        }
    }
    lseek(fdout, src_statbuf.st_size - 1, SEEK_SET);
    write(fdout, "", 1);
    char * in;
    char * out;
    in = mmap(0, src_statbuf.st_size, PROT_READ, MAP_SHARED, fdin, 0);
    out = mmap(0, src_statbuf.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fdout, 0);
    memcpy(out, in, src_statbuf.st_size);
    close(fdin);
    close(fdout);
    return 0;
}