#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include <errno.h>
#include<sys/wait.h>


int invalid_input_check(char const *buf){
    int i = 0;
    int flag = 0;
    int begin_flag = 1;
    while (buf[i] != '\0')
    {
        if ((buf[i] == '|' && flag == 1))
        {
            printf("Incorrect input\n");
            return 1;
        }
        if (buf[i] == '|' && flag == 0)
        {
            if (begin_flag == 1)
            {
                printf("Incorrect input\n");
                return 1;
            }
            flag = 1;
        }
        if (buf[i] != '|' && buf[i] != ' ')
        {
            begin_flag = 0;
            flag = 0;
        }
        if (buf[i] == '|' && buf[i + 2] == '\0')
        {
            printf("Incorrect input\n");
            return 1;
        }
        i++;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    char *words[100000];
    char *arg[100000];
    long int i = 0, j = 0, k = 0, child_proc_num = 0;
    char buf[10000];
    pid_t id;
    int save_in;
    int character;
    int fp[100][2];
    while (1) {

        printf("&& ");
        i = 0;

        while ((character = getchar()) == ' ' && character != '\n') {}

        if (character == '\n'){
            continue;
        }

        buf[0] = (char) character;
        i++;

        while ((character = getchar()) != '\n') {

            if (character == EOF) {
                return 0;
            }
            if (i > 0 && character != ' ' && buf[i - 1] == '|') {
                buf[i] = ' ';
                i++;
            }
            if (i > 0 &&  character == '|' && buf[i - 1] != ' ') {
                buf[i] = ' ';
                i++;
            }
            if (i > 0 && (character != ' ') && buf[i - 1] == '|') {
                buf[i] = ' ';
                i++;
            }
            if (i > 0 && (character == ' ' && buf[i - 1] == ' ')) {
                continue;
            }
            buf[i] = (char) character;
            i++;
        }
        buf[i] = '\0';
        i++;

        if (i > 0 && buf[i - 1] == '|') {
            buf[i] = ' ';
            i++;
        }


        if (invalid_input_check(buf) == 1) {
            continue;
        }

        i = 0;
        words[0] = strtok(buf, " ");
        arg[0] = words[0];
        while (words[i] != NULL) {
            i++;
            words[i] = strtok(NULL, " ");
            arg[i] = words[i];
        }

        if (strcmp(words[0], "exit") == 0) {
            return 0;
        }
        j = 0;
        child_proc_num = 0;
        save_in = dup(0);
        // save_in использую для сохранения целостности stdin
        while (words[j] != NULL) {
            k = 0;
            while (!(words[j] == NULL || strcmp(words[j], "|") == 0)) {
                arg[k] = words[j];
                k++;
                j++;
                if (words[j] == NULL) {
                    j--;
                    break;
                }
            }
            arg[k] = NULL;
            j++;
            // Данный pipe использую для передачи
            // из дочернего процесса в родительский
            // информации об полученной в exec ошибке
            if (i == 1 || words[j] == NULL) {
                id = fork();
                if (id == 0) {
                    if (execvp(arg[0], arg) < 0) {
                        perror("Oh, dear, sonething went wrong");
                    }
                    return 0;
                }
                break;
            } else {
                pipe(fp[child_proc_num]);
                // Pipe связывающий stdin и stdout
                // команд, разделенных палочкой
                id = fork();
                if (id == 0) {
                    close(fp[child_proc_num][0]);
                    dup2(fp[child_proc_num][1], 1);
                    if (execvp(arg[0], arg) < 0) {
                        perror("Oh, dear, sonething went wrong");
                    }
                    return 0;
                }
                close(fp[child_proc_num][1]);
                dup2(fp[child_proc_num][0], 0);
                close(fp[child_proc_num][0]);
                child_proc_num++;
            }

        }
        // В child_proc_num считаю количесвто
        // открытых процессов и после всех exec
        // жду их окончания
        while (child_proc_num >= 0) {
            child_proc_num--;
            wait(NULL);
        }
        dup2(save_in, 0);
        close(save_in);
    }
    return 0;
}
