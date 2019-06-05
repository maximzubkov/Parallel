#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <signal.h>
#include <time.h>
#include <signal.h>


#define ON_THE_LEFT 0
#define ON_THE_RIGHT 1
#define BRIDGE_BLOCK 2
#define HAT_STATUS 3
#define FLOW_STATUS 4
#define LAST_LEFT 5
#define LAST_RIGHT 6
#define PROC_LEFT 0
#define PROC_RIGHT 1


int sem_status;
int sem_block;
int sem_num;


int main(int argc, char * argv[]) {
    int on_the_left = atoi(argv[1]);
    int on_the_right = atoi(argv[2]);
    int i = 0;

    struct sembuf proc_left_increse;
    proc_left_increse.sem_num = PROC_LEFT;
    proc_left_increse.sem_op = 1;
    proc_left_increse.sem_flg = 0;

    struct sembuf proc_right_increse;
    proc_right_increse.sem_num = PROC_RIGHT;
    proc_right_increse.sem_op = 1;
    proc_right_increse.sem_flg = 0;


    struct sembuf unblock_right;
    unblock_right.sem_num = ON_THE_RIGHT;
    unblock_right.sem_op = 1;
    unblock_right.sem_flg = 0;

    struct sembuf block_right;
    block_right.sem_num = ON_THE_RIGHT;
    block_right.sem_op = -1;
    block_right.sem_flg = 0;


    struct sembuf block_left;
    block_left.sem_num = ON_THE_LEFT;
    block_left.sem_op = -1;
    block_left.sem_flg = 0;

    struct sembuf unblock_left;
    unblock_left.sem_num = ON_THE_LEFT;
    unblock_left.sem_op = 1;
    unblock_left.sem_flg = 0;

    struct sembuf unblock_hat_and_left[2];
    unblock_hat_and_left[0].sem_num = HAT_STATUS;
    unblock_hat_and_left[0].sem_op = -1;
    unblock_hat_and_left[0].sem_flg = 0;
    unblock_hat_and_left[1].sem_num = ON_THE_LEFT;
    unblock_hat_and_left[1].sem_op = 1;
    unblock_hat_and_left[1].sem_flg = 0;

    struct sembuf unblock_flow_and_left[2];
    unblock_flow_and_left[0].sem_num = FLOW_STATUS;
    unblock_flow_and_left[0].sem_op = -1;
    unblock_flow_and_left[0].sem_flg = 0;
    unblock_flow_and_left[1].sem_num = ON_THE_LEFT;
    unblock_flow_and_left[1].sem_op = 1;
    unblock_flow_and_left[1].sem_flg = 0;

    struct sembuf hat_unblock;
    hat_unblock.sem_num = HAT_STATUS;
    hat_unblock.sem_op = -1;
    hat_unblock.sem_flg = 0;

    struct sembuf wait_hat_unblock_and_lock_bridge[2];
    wait_hat_unblock_and_lock_bridge[0].sem_num = HAT_STATUS;
    wait_hat_unblock_and_lock_bridge[0].sem_op = 0;
    wait_hat_unblock_and_lock_bridge[0].sem_flg = 0;
    wait_hat_unblock_and_lock_bridge[1].sem_num = BRIDGE_BLOCK;
    wait_hat_unblock_and_lock_bridge[1].sem_op = 1;
    wait_hat_unblock_and_lock_bridge[1].sem_flg = 0;

    struct sembuf wait_bridge_unblock_and_lock_hat[2];
    wait_bridge_unblock_and_lock_hat[0].sem_num = BRIDGE_BLOCK;
    wait_bridge_unblock_and_lock_hat[0].sem_op = 0;
    wait_bridge_unblock_and_lock_hat[0].sem_flg = 0;
    wait_bridge_unblock_and_lock_hat[1].sem_num = HAT_STATUS;
    wait_bridge_unblock_and_lock_hat[1].sem_op = 1;
    wait_bridge_unblock_and_lock_hat[1].sem_flg = 0;

    sem_num = semget(IPC_PRIVATE, 2, IPC_CREAT | 0660);
    semctl(sem_num, PROC_LEFT, SETVAL, 0);
    semctl(sem_num, PROC_RIGHT, SETVAL, 0);

    sem_block = semget(IPC_PRIVATE, 5, IPC_CREAT | 0660);
    semctl(sem_block, ON_THE_LEFT, SETVAL, 1);
    semctl(sem_block, ON_THE_RIGHT, SETVAL, 1);
    semctl(sem_block, BRIDGE_BLOCK, SETVAL, 0);
    semctl(sem_block, HAT_STATUS, SETVAL, 0);
    semctl(sem_block, FLOW_STATUS, SETVAL, 1);

    for (i = 0; i < on_the_left; i++){
        if(fork() == 0){
            semop(sem_num, &proc_left_increse, 1);
            int val = semctl(sem_num, PROC_LEFT, GETVAL);
            //printf("left %d fork\n", val);
            semop(sem_block, &block_left, 1);
            if(semctl(sem_block, FLOW_STATUS, GETVAL) == 1) {
                printf("(left ship_keeper %d) on left UNBLOCKED\n", val);
                printf("(left ship_keeper %d) on right LOCKING_HAT\n", val);
                semop(sem_block, wait_bridge_unblock_and_lock_hat, 2);
                printf("(left ship_keeper %d) on left TAKE_HERD\n", val);
                printf("(left ship_keeper %d) on right WITH_HERD, UNLOCKING_FLOW\n", val);
                semop(sem_block, unblock_flow_and_left, 2);
                semop(sem_block, unblock_hat_and_left, 2);
            } else {
                printf("(left ship_keeper %d) on right WITH_HERD\n", val);
                if (val == on_the_left + 1){
                    semop(sem_block, &hat_unblock, 2);
                    printf("(left ship_keeper %d) on right SAYS_GOODBAY\n", val);
                }
                semop(sem_block, &unblock_left, 1);
            }
            return 0;
        }
    }
    for (i = 0; i < on_the_right; i++){
        if(fork() == 0){
            semop(sem_num, &proc_right_increse, 1);
            int val = semctl(sem_num, PROC_RIGHT, GETVAL);
            semop(sem_block, &block_right, 1);
            printf("right %d fork unblocked\n", val);
            printf("(right ship_keeper %d) on right UNBLOCKED\n", val);
            semop(sem_block, wait_hat_unblock_and_lock_bridge, 2);
            printf("(right ship_keeper %d) on right TAKING_HERD\n", val);
            if (val == on_the_right) {
                printf("(right ship_keeper %d) on left SAYS_GOODBAY\n", val);
                semop(sem_block, &unblock_right, 1);
            } else {
                semop(sem_block, &unblock_right, 1);
            }
            return 0;
        }
    }
    for (i = 0; i < on_the_left + on_the_right; i++){
        wait(NULL);
    }
    return 0;
}