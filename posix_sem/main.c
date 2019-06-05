#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <semaphore.h>
#include <unistd.h>
#include <math.h>
#include <strings.h>
#include <stdlib.h>

static sem_t * sem_read; // семафор, отвечающий за блокировку read
static sem_t * sem_write; // семафор, отвечающий за блокировку write
static sem_t * sem_end; // семафор конца read
static sem_t * read_status; // статус read, который необходимо сообщить в функцию manager
static sem_t * write_status; // статус write, который необходимо сообщить в функцию manager
static sem_t * sem_manager; // семафор, отвечающий за блокировку manager


struct node {
    char * val;
    struct node * prev;
    struct node *next;
};

struct query {
    int bits;
    int max;
    int currently;
    int first;
    int last;
    struct node ** a;
};

struct query * query_create (struct query * q, int bit_in_element, int capacity_of_buf) {
    int i;
    q = (struct query *) malloc(sizeof(struct query));
    q->a = (struct node **) malloc(capacity_of_buf * sizeof(struct node));
    for (i = 0; i < capacity_of_buf; i++) {
        q->a[i] = (struct node *) malloc(sizeof(struct node));
        (q->a[i])->val = (char *) malloc(bit_in_element * sizeof(char));
    }
    for (i = 1; i < capacity_of_buf - 1; i++) {
        q->a[i]->prev = q->a[i - 1];
        q->a[i]->next = q->a[i + 1];
    }
    q->a[0]->prev = q->a[capacity_of_buf - 1];
    q->a[0]->next = q->a[1];
    q->a[capacity_of_buf - 1]->prev = q->a[capacity_of_buf - 2];
    q->a[capacity_of_buf - 1]->next = q->a[0];
    q->first = 0;
    q->last = -1;
    q->bits = bit_in_element;
    q->max = capacity_of_buf;
    q->currently = 0;
    return q;
}


void query_push(struct query * q, struct node * x) {
    if (q->currently == q->max) {
        printf("SOMETHING WRONG!!!! QUERY IS FULL!!!!\n");
    } else {
        q->last = (q->last + 1) % q->max;
        q->currently++;
        memcpy((q->a[q->last])->val, x->val, q->bits);
    }
}


struct node * query_pop(struct query * q) {
    if (q->currently != 0) {
        int to_return = q->first;
        q->first = (q->first + 1) % q->max;
        q->currently--;
        return q->a[to_return];
    } else {
        return NULL;
    }
}


void * manager(void * param){
    struct query * q = (struct query *)param;
    int * end = (int *)malloc(sizeof(int));
    int * read_v = (int *)malloc(sizeof(int));
    int * write_v = (int *)malloc(sizeof(int));
    int * proc = (int *)malloc(sizeof(int) * 1000);
    char * err = (char *)malloc(sizeof(char) * 3);
    int i = 0;
    int j = 0;
    size_t size = 1000;
    while(1) {
        if (i == size){
            proc = (int *)realloc(proc, sizeof(int) * (size + 1000));
        }
        proc[i] = 100 * q->currently/q->max;
        i++;
        sem_getvalue(sem_end, end);
        if(*end == 0){
            /* Статистика заполнености
             */
            for(j = 0; j < i; j++){
                err[0] = (char)((proc[j]/10) %10 + '0');
                err[1] = (char)((proc[j]) %10 + '0');
                write(STDERR_FILENO, "query filled in " , strlen("query filled in "));
                write(STDERR_FILENO, err , 2);
                write(STDERR_FILENO, "% " , 2);

            }
            free(end);
            free(write_v);
            free(read_v);
            free(err);
            free(proc);
            sem_post(sem_write);
            return NULL;
        }

        sem_wait(sem_manager);
        /* При каждой новой итеррации цикла блокируем manager
         * до тех пор, пока не слуится граничная ситуация,
         * то есть до тех пор пока 0.2 <= q->current/q->max <= 0.8
         */

        sem_getvalue(write_status, write_v);
        sem_getvalue(read_status, read_v);
        // Если во время write мы попали в ситуацию q->current/q->max <= 0.8, то:
        if (*write_v == 1 && *read_v == 0){
            sem_post(read_status);
            sem_post(sem_write);
            sem_post(sem_read);
            continue;
        }
        // Если во время read мы попали в ситуацию q->current/q->max >= 0.2, то:
        if (*write_v == 0 && *read_v == 1){
            sem_post(write_status);
            sem_post(sem_read);
            sem_post(sem_write);
            continue;
        }
    }
}

void * reader(void * param) {
    struct query * q = (struct query *)param;
    struct node * x;
    int * status = (int *)malloc(sizeof(int));
    int current_val = 0;
    int i;

    x = (struct node *)malloc(sizeof(struct node));
    x->val = (char *)malloc(q->bits * sizeof(char));

    while(1) {
        sem_getvalue(read_status, status);
        if (q->currently > 0.8 * q->max && *status == 1) {
            sem_wait(read_status);
            sem_wait(sem_read);
        }

        current_val = (int)read(STDIN_FILENO, x->val, (size_t) q->bits);

        if (current_val <= 0) {
            sem_wait(sem_end);
            free(x->val);
            free(x);
            free(status);
            sem_post(sem_manager);
            sem_post(sem_write);
            return NULL;
        }

        if (current_val < q->bits){
            for (i = current_val; i < q->bits; i++) {
                x->val[i] = '\0';
            }
        }

        query_push(q, x);
        sem_getvalue(write_status, status);
        if (q->currently > 0.2 * q->max && *status == 0) {
            /* К этому моменту write уже заблокирован,
             * поэтому мы открываем manager и блокируем read
             */
            sem_post(sem_manager);
            sem_wait(sem_read);
        }
    }
}

void * writer(void * param){
    struct query * q = (struct query *)param;
    //struct node * x = (struct node *)malloc(sizeof(struct node));
    //x->val = (char *)malloc(args->q->bits * sizeof(char));
    struct node * x;
    int * status = (int *)malloc(sizeof(int));
    int * end = (int *)malloc(sizeof(int));
    int i = 1;
    while(1) {
        sem_getvalue(sem_end, end);
        if (*end == 1) {
            /* В случае, если end == 0, значит весь stdin прочитан
             * поэтому stdout не нужно закрывать при q->current/q->max <= 0.2
             */
            sem_getvalue(write_status, status);
            if (q->currently < 0.2 * q->max && *status == 1) {
                sem_wait(write_status);
                sem_wait(sem_write);
            }
        }
        // query_pop выдает NULL, если q->current == 0
        x = query_pop(q);
        if(x == NULL) {
            free(status);
            free(end);
            //free(x->val);
            //free(x);
            return NULL;
        }
        while(x->val[q->bits - i] == '\0'){
            i++;
        }
        write(STDOUT_FILENO, x->val, q->bits - i + 1);
        i = 1;
        sem_getvalue(sem_end, end);
        if (*end == 1) {
            sem_getvalue(read_status, status);
            if (q->currently < 0.8 * q->max && *status == 0) {
                /* К этому моменту write уже заблокирован,
                 * поэтому мы открываем manager и блокируем read
                 */
                sem_post(sem_manager);
                sem_wait(sem_write);
            }
        }
    }
}


int main(int argc, char * argv[]) {
    int max_bit = atoi(argv[1]);
    int max_elements = atoi(argv[2]);

    pthread_t read_thread;
    pthread_t write_thread;
    pthread_t manager_thread;

    struct query * q = (struct query *)malloc(sizeof(struct query));
    q = query_create(q, max_bit, max_elements);


    sem_read = (sem_t *)malloc(sizeof(sem_t));
    if(sem_init(sem_read, 0, 0) == -1){
        perror("init of sem_read error:");
        return 0;
    }
    sem_write = (sem_t *)malloc(sizeof(sem_t));
    if(sem_init(sem_write, 0, 0) == -1){
        perror("init of sem_write error:");
        return 0;
    }
    /* Для семафора sem_end ноль означает, что все,
     * что можно было прочитать уже прочитано, а единица,
     * что чтение еще идет
     */
    sem_end = (sem_t *)malloc(sizeof(sem_t));
    if(sem_init(sem_end, 0, 1) == -1){
        perror("init of sem_end error:");
        return 0;
    }
    /* Для семафоров, отвечабщих за статус
     * изначально установленное значение - единица
     * означает, unlocked, а ноль - locked
     */
    write_status = (sem_t *)malloc(sizeof(sem_t));
    if(sem_init(write_status, 0, 1) == -1){
        perror("init of write_status error:");
        return 0;
    }
    read_status = (sem_t *)malloc(sizeof(sem_t));
    if(sem_init(read_status, 0, 1) == -1){
        perror("init of read_status error:");
        return 0;
    }
    sem_manager = (sem_t *)malloc(sizeof(sem_t));
    if(sem_init(sem_manager, 0, 0) == -1){
        perror("init of sem_manager error:");
        return 0;
    }

    pthread_create(&manager_thread, NULL, (void *)manager, (void *)q);
    pthread_create(&read_thread, NULL, (void *)reader, (void *)q);
    pthread_create(&write_thread, NULL, (void *)writer, (void *)q);

    pthread_join(read_thread, NULL);
    pthread_join(manager_thread, NULL);
    pthread_join(write_thread, NULL);

    pthread_cancel(read_thread);
    pthread_cancel(manager_thread);
    pthread_cancel(write_thread);

    sem_destroy(sem_manager);
    sem_destroy(sem_read);
    sem_destroy(sem_write);
    sem_destroy(write_status);
    sem_destroy(read_status);
    sem_destroy(sem_end);

    int i = 0;
    for(i = 0; i < q->max; i++){
        free(q->a[i]->val);
        free(q->a[i]);
    }
    free(q->a);
    free(q);
    return 0;
}