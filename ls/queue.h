//
// Created by Максим on 25.10.2018.
//

#ifndef LS_BETA_QUEUE_H
#define LS_BETA_QUEUE_H

#endif //LS_BETA_QUEUE_H

int mycmp(const char *a, const char *b) {
    const char *cp1 = a, *cp2 = b;

    for (; (*cp1) == (*cp2); cp1++, cp2++)
        if (*cp1 == '\0')
            return 0;
    return (((*cp1) < (*cp2)) ? -1 : 1);
}

int cmp(const void *p1, const void *p2){
    return mycmp(* (char * const *) p1, * (char * const *) p2);
}


typedef struct directory Data;

struct queue {
    int n;
    char * str[10000];
};

struct queue * q_create(int size) {
    struct queue * q = (struct queue *) malloc(size * sizeof(struct queue));
    q->n = 0;
    return q;
}
void q_pushback(struct queue * q, char * x) {
    q->str[q->n] = (char *) malloc(10000);
    strcpy(q->str[q->n], x);
    q->n++;
}
void q_pushfront(struct queue * q, char x[]) {
    int i = 0;
    q->str[q->n] = (char *) malloc(10000);
    for (i = q->n; i > 0; i--) {
        strcpy(q->str[i], q->str[i - 1]);
    }
    strcpy(q->str[0], x);
    q->n++;
}
int q_empty(struct queue * q) {
    return q->n ? 0 : 1;
}

void q_pop (char pop[], struct queue * q) {
    int i;
    strcpy(pop, q->str[0]);
    for (i = 0; i < q->n - 1; i++) {
        strcpy(q->str[i], q->str[i + 1]);
    }
    q->n--;
}

void q_print(struct queue *q) {
    int i;
    for (i = 0; i < q->n; i++) {
        printf("%s ", q->str[i]);
    }
}

