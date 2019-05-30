#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <time.h>
#include <grp.h>
#include <errno.h>
#include "queue.h"
#include "flag_print.h"

int flag = 1;

struct queue * q;

void str_dir_cat (char name[], const char * tail, const char * head)
{
    // Функция для того, чтобы к уже
    // имеющимуся пути head добавить "/tail"
    strcpy(name, tail);
    *name = *strcat(name, "/");
    *name = *strcat(name, head);
}

int dirwork(char *name, struct flag_struct * f) {//Работа с директориями
    DIR *dir;
    struct stat buf;
    struct dirent *entry;
    char path[1025];
    //int colomn = 1;
    //int num = 0;
    if (f->d_flag == 1) {
        printf("%s\n", name);
        if (f->l_flag == 1) {
            if_l(name, f->n_flag);
        }
        if (f->n_flag == 1) {
            if_n(name);
        }
        if (f->i_flag == 1) {
            if_i(name);
        }
        return 0;
    }
    dir = opendir(name);
    while ((entry = readdir(dir)) != NULL) {// Цикл чтения
        if (f->d_flag) {
            printf("%s", name);
            get_stat(name, f);
            break;
        }
        if (entry->d_name[0] == '.') {
            if (f->a_flag) {
                printf("%s", entry->d_name);
                get_stat(name, f);
                continue;
            }
            if (f->R_flag == 1)
            {
                continue;
            }
        } else {
            if (f->R_flag)// Рекурсия
            {
                str_dir_cat(path, name, entry->d_name);// Создание абсолютного пути
                if (stat(name, &buf) < 0) {
                    printf("ERROR\n");
                    return 0;
                }
                if (S_ISDIR(buf.st_mode)) {
                    if (flag) {
                        q_pushback(q, path);
                    }
                    else
                        q_pushfront(q, path);
                }
            }
        }
        if(f->n_flag == 1 || f->l_flag == 1 || f->i_flag == 1)
        {
            get_stat(name, f);
            printf("%s\n", entry->d_name);
        }
        else {
            // num = 0;
            /*if (colomn % 8 == 0) {
                printf("\n");
            }
            while (entry->d_name[num] != '\0') {
                num++;
            }*/
            printf("%s\n", entry->d_name);
            /*while (24 - num > 0) {
                printf(" ");
                num++;
            }
            colomn++;*/
        }
    }
    printf("\n");
    closedir(dir);
    flag = 0;
    while (q_empty(q) == 0) {
        char nema[100];
        q_pop(nema, q);
        dir = opendir(nema);
        if (dir != NULL) {
            printf("\n%s:\n", nema);
            dirwork(nema, f);
        }
    }
    return 0;
}


int main(int argc, char *argv[]) {
    struct flag_struct * f = f_create();
    char c;
    q = q_create(10000);
    while ((c = getopt(argc, argv, "alnRid")) != -1)// Проверка флагов
    {
        switch (c) {
            case 'a':
                f->a_flag = 1;
                break;
            case 'l':
                f->l_flag = 1;
                break;
            case 'n':
                f->n_flag = 1;
                break;
            case 'R':
                f->R_flag = 1;
                break;
            case 'i':
                f->i_flag = 1;
                break;
            case 'd':
                f->d_flag = 1;
                break;
        }
    }
    char * curr_dir = getenv("PWD");
    int i = 0;
    int no_dir = 1;
    struct stat info;
    for (i = 1; i < argc; i++) {// Проверка существование директорий или файлов среди аргументов, с последующим ls
        if (argv[i][0] != '-') {
            if (stat(argv[i], &info) != 0) {
                perror("Stat_error");
                no_dir = 0;
            }
            if (S_ISDIR(info.st_mode)) {
                dirwork(argv[i], f);
                no_dir = 0;
            } else if (S_ISREG(info.st_mode)) {
                printf("%s", argv[i]);
                no_dir = 0;
            }
            if(argc - 1 != i) {
                printf("\n");
            }
        }
    }
    //f->n_flag = 1;
    //f->R_flag = 1;
    //strcpy(curr_dir, "./test");
    if (no_dir == 1)
        dirwork(curr_dir, f);
    printf("\n");
    return 0;
}