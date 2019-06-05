#include <stdio.h>
#include <errno.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/msg.h>

int ms;

struct msqid_ds buf;

struct msgbuf{
    long type;
    char data[5];
};

int n;

int runner (int N)
{
    printf ("%d пришел\n", N);

    if (N > 500)
    {
        msgctl(ms, IPC_STAT, &buf);
        printf("q: %lu, num: %lu\n", buf.msg_qbytes, buf.msg_qnum);
    }
    struct msgbuf come_struct;
    come_struct.type = N;
    strcpy(come_struct.data, "come");
    if(msgsnd(ms, (void *) &come_struct, sizeof(come_struct.data), MSG_NOERROR) == -1)
        perror("Oh, dear, sonething went wrong");
    msgctl(ms, IPC_STAT, &buf);
    printf("q: %lu, num: %lu\n", buf.msg_qbytes, buf.msg_qnum);

    struct msgbuf start_struct;
    if(msgrcv(ms, (void *) &start_struct, sizeof(start_struct.data), n + N, MSG_NOERROR) == -1)
        perror("Oh, dear, sonething went wrong");

    //printf ("%d побежал\n", N);

    struct msgbuf finish_struct;
    finish_struct.type = N + n + 1;
    strcpy(finish_struct.data, "palk");
    //printf("%d добежал\n", N);
    if(msgsnd(ms, (void *) &finish_struct, sizeof(come_struct.data), MSG_NOERROR) == -1)
        perror("Oh, dear, sonething went wrong");
    return 0;

}
int judge (int N)
{
    printf("Пришел на стадион\n");

    struct msgbuf start_struct;

    int j = 1;
    while (j != N + 1) {
        if(msgrcv(ms, (void *) &start_struct, sizeof(start_struct.data), -N - 1, MSG_NOERROR) == -1)
            perror("Oh, dear, sonething went wrong");
        j++;
    }
    struct msgbuf zero_struct;
    zero_struct.type = n + 1;
    strcpy(zero_struct.data, "palk");
    printf("Дал палочку первому бегуну\n");
    if(msgsnd(ms, (void *) &zero_struct, sizeof(zero_struct.data), MSG_NOERROR) == -1)
        perror("Oh, dear, sonething went wrong");
    struct timeval begin;
    gettimeofday(&begin, NULL);

    struct msgbuf finish_struct;
    if(msgrcv(ms, (void *) &finish_struct, sizeof(finish_struct.data), 1 + n + N, MSG_NOERROR) == -1)
        perror("Oh, dear, sonething went wrong");

    struct timeval end;
    gettimeofday(&end, NULL);

    printf("Забрал палочку у последнего, время пробежки: %f\n", (end.tv_sec - begin.tv_sec) + (end.tv_usec - begin.tv_usec)/1000000.0);

    msgctl(ms, IPC_RMID, &buf);

    printf("Ушел домой\n");
    return 0;
}
int main(int argc, char *argv[]) {
    pid_t f;
    n = atoi(argv[1]);
    int i = 0;
    ms = msgget(IPC_PRIVATE, 0777 | IPC_CREAT | IPC_EXCL);
    msgctl(ms, IPC_STAT, &buf);
    printf("q: %lu, num: %lu\n", buf.msg_qbytes, buf.msg_qnum);
    buf.msg_qbytes = (unsigned int)(n * sizeof(struct msgbuf));
    msgctl(ms, IPC_SET, &buf);
    if (fork() == 0) {
        judge(n);
        return 0;
    }
    pid_t id;
    for (i = 1; i <= n; i++) {
        id = fork();
        if (id == 0) {
            runner(i);
            return 0;
        }
    }
    f = getpid();
    wait(&f);
    for (i = 1; i <= n; i++)
    {
        wait(&f);
    }
    return 0;
}