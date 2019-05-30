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


#define BLOCK_BOAT_FLOW 0 // Поток людей с корабля на трап и с трапа на корабль
#define BLOCK_LAND_FLOW 1 // Поток людей с суши на трап и с трапа на сушу
#define ON_BOAT 2 //Людей на борту
#define ON_LAND 3 //Людей на земле
#define SITS_LEFT 4 //Осталось мест на борту
#define BREAK 5
#define BLOCK_TRAP 0
#define BLOCK_BOARDING 1
#define BLOCK_LANDING 2
#define BLOCK_SHIP 3
#define SHIP_STATUS 0
#define TRAP_STATUS 1
#define BOARDING_STATUS 2
#define LANDING_STATUS 3
#define END_STATUS 4



int sem_status; //Массив семафоров на статутс децствий
int sem_block; //Массив семафоров блокирующих разные действия
int sem_amount_block; //Семафор на количество людей на трапе

struct args {
    int people_val;
    int trap_val;
    int trip_val;
    int capesity_of_ship;
};

// На вход общее число людей, число людей, которых может выдержать трап, число путешествий корабля, количество людей на корабле
int main(int argc, char * argv[]) {
    struct args arg;
    arg.people_val = atoi(argv[1]);
    arg.trap_val = atoi(argv[2]);
    arg.trip_val = atoi(argv[3]);
    arg.capesity_of_ship = atoi(argv[4]);

    /* Семафороы для блокировки соответсвующих процессов */

    sem_block = semget(IPC_PRIVATE, 4, IPC_CREAT | 0660);
    semctl(sem_block, BLOCK_BOARDING, SETVAL, 0);
    semctl(sem_block, BLOCK_LANDING, SETVAL, 0);
    semctl(sem_block, BLOCK_TRAP, SETVAL, 0);
    semctl(sem_block, BLOCK_SHIP, SETVAL, 0);

    /* Массив семафоров для блокировки
     * посадки или высадки в случае переполнения трапа,
     * переполнения корабля или выхода за границу допустимых людей на суше
     */

    sem_amount_block = semget(IPC_PRIVATE, 6, IPC_CREAT | 0660);

    /* В дальшейшем при поступлении нового человека на трап, мы будем увелиивть значение
     * sem_amount_block[BLOCK_BOAT_FLOW] и уменьшать sem_amount_block[BLOCK_LAND_FLOW] */

    semctl(sem_amount_block, BLOCK_BOAT_FLOW, SETVAL, 0); //Для блокировки процесса, пытающегося снять человека с трапа, когда трап пуст
    semctl(sem_amount_block, BLOCK_LAND_FLOW, SETVAL, arg.trap_val); //Для блокировки процесса, пытающегося поместить нового человека на трап, когда трап переполнен
    semctl(sem_amount_block, ON_BOAT, SETVAL, 0);//Для блокировки процесса, пытающегося снять с корабля человека, когда на корабле никого нет
    semctl(sem_amount_block, ON_LAND, SETVAL, arg.people_val); //Для блокировки процесса, пытающегося забрать с земли человека, когда на земле уже никого нет
    semctl(sem_amount_block, SITS_LEFT, SETVAL, arg.capesity_of_ship); //Для блокировки процесса, пытающегося поместить на борт человека, когда места на борту уже нет
    semctl(sem_amount_block, BREAK, SETVAL, 0);

    /* Блокировка корабля во время прибывания на суше */

    sem_status = semget(IPC_PRIVATE, 5, IPC_CREAT | 0660);
    semctl(sem_status, SHIP_STATUS, SETVAL, 1);
    semctl(sem_status, TRAP_STATUS, SETVAL, 1);
    semctl(sem_status, BOARDING_STATUS, SETVAL, 1);
    semctl(sem_status, LANDING_STATUS, SETVAL, 1);
    semctl(sem_status, END_STATUS, SETVAL, 0);


    //Буфер для атомарной блокировки семафоров посадки и высадки (а также для изменения статуса посадки/высадки)
    struct sembuf block_boarding_buf;
    block_boarding_buf.sem_num = BLOCK_BOARDING;
    block_boarding_buf.sem_op = -1;
    block_boarding_buf.sem_flg = 0;

    struct sembuf block_landing_buf;
    block_landing_buf.sem_num = BLOCK_LANDING;
    block_landing_buf.sem_op = -1;
    block_landing_buf.sem_flg = 0;

    struct sembuf unblock_landing_buf;
    unblock_landing_buf.sem_num = BLOCK_LANDING;
    unblock_landing_buf.sem_op = 1;
    unblock_landing_buf.sem_flg = 0;

    struct sembuf unblock_boarding_buf;
    unblock_boarding_buf.sem_num = BLOCK_BOARDING;
    unblock_boarding_buf.sem_op = 1;
    unblock_boarding_buf.sem_flg = 0;

    //Буфер для атомарной разблокировки семафоров посадки и высадки одновременно (а также изменения их статуса)
    struct sembuf unblock_flow_buf[2];
    unblock_flow_buf[0].sem_num = BLOCK_BOARDING;
    unblock_flow_buf[0].sem_op = 1;
    unblock_flow_buf[0].sem_flg = 0;
    unblock_flow_buf[1].sem_num = BLOCK_LANDING;
    unblock_flow_buf[1].sem_op = 1;
    unblock_flow_buf[1].sem_flg = 0;

    struct sembuf block_flow_buf[2];
    block_flow_buf[0].sem_num = BLOCK_BOARDING;
    block_flow_buf[0].sem_op = -1;
    block_flow_buf[0].sem_flg = 0;
    block_flow_buf[1].sem_num = BLOCK_LANDING;
    block_flow_buf[1].sem_op = -1;
    block_flow_buf[1].sem_flg = 0;

    //Буфер для атомарной разблокировки трапа
    struct sembuf unblock_trap_buf;
    unblock_trap_buf.sem_num = BLOCK_TRAP;
    unblock_trap_buf.sem_op = 1;
    unblock_trap_buf.sem_flg = 0;

    //Буфер для атомарной блокировки трапа
    struct sembuf block_trap_buf;
    block_trap_buf.sem_num = BLOCK_TRAP;
    block_trap_buf.sem_op = -1;
    block_trap_buf.sem_flg = 0;

    //Буфер для атомарной блокировки корабля (или изменения статуса корабля)
    struct sembuf block_ship_buf;
    block_ship_buf.sem_num = BLOCK_SHIP;
    block_ship_buf.sem_op = -1;
    block_ship_buf.sem_flg = 0;

    //Буфер для атомарной разблокировки корабля (или изменения статуса корабля)
    struct sembuf unblock_ship_buf;
    unblock_ship_buf.sem_num = BLOCK_SHIP;
    unblock_ship_buf.sem_op = 1;
    unblock_ship_buf.sem_flg = 0;

    //Буфер для атомарного уменьшения статуса корабля
    struct sembuf block_ship_status_buf;
    block_ship_status_buf.sem_num = SHIP_STATUS;
    block_ship_status_buf.sem_op = -1;
    block_ship_status_buf.sem_flg = 0;

    //Буфер для атомарной увеличения статуса корабля
    struct sembuf unblock_ship_status_buf;
    unblock_ship_status_buf.sem_num = SHIP_STATUS;
    unblock_ship_status_buf.sem_op = 1;
    unblock_ship_status_buf.sem_flg = 0;

    //Буфер для атомарного уменьшения статуса концв
    struct sembuf block_end_buf;
    block_end_buf.sem_num = END_STATUS;
    block_end_buf.sem_op = -1;
    block_end_buf.sem_flg = 0;

    //Буфер для атомарной увеличения статуса конца
    struct sembuf unblock_end_buf;
    unblock_end_buf.sem_num = END_STATUS;
    unblock_end_buf.sem_op = 1;
    unblock_end_buf.sem_flg = 0;

    //Буфер для атомарной увеличения статуса конца
    struct sembuf unblock_landing_status_buf;
    unblock_landing_status_buf.sem_num = LANDING_STATUS;
    unblock_landing_status_buf.sem_op = 1;
    unblock_landing_status_buf.sem_flg = 0;

    //Буфер для атомарного уменьшения статуса концв
    struct sembuf block_landing_status_buf;
    block_landing_status_buf.sem_num = LANDING_STATUS;
    block_landing_status_buf.sem_op = -1;
    block_landing_status_buf.sem_flg = 0;

    //Буфер для атомарной увеличения статуса конца
    struct sembuf unblock_boarding_status_buf;
    unblock_boarding_status_buf.sem_num = BOARDING_STATUS;
    unblock_boarding_status_buf.sem_op = 1;
    unblock_boarding_status_buf.sem_flg = 0;

    //Буфер для атомарного уменьшения статуса концв
    struct sembuf block_boarding_status_buf;
    block_boarding_status_buf.sem_num = BOARDING_STATUS;
    block_boarding_status_buf.sem_op = -1;
    block_boarding_status_buf.sem_flg = 0;


    //Буфер для атомарного уменьшения статуса flow
    struct sembuf block_flow_status_buf[2];
    block_flow_status_buf[0].sem_num = LANDING_STATUS;
    block_flow_status_buf[0].sem_op = -1;
    block_flow_status_buf[0].sem_flg = 0;
    block_flow_status_buf[1].sem_num = BOARDING_STATUS;
    block_flow_status_buf[1].sem_op = -1;
    block_flow_status_buf[1].sem_flg = 0;

    //Буфер для атомарной увеличения статуса flow
    struct sembuf unblock_flow_status_buf[2];
    unblock_flow_status_buf[0].sem_num = LANDING_STATUS;
    unblock_flow_status_buf[0].sem_op = 1;
    unblock_flow_status_buf[0].sem_flg = 0;
    unblock_flow_status_buf[1].sem_num = BOARDING_STATUS;
    unblock_flow_status_buf[1].sem_op = 1;
    unblock_flow_status_buf[1].sem_flg = 0;


    //Буфер для атомарного уменьшения статуса trap
    struct sembuf block_trap_status_buf;
    block_trap_status_buf.sem_num = TRAP_STATUS;
    block_trap_status_buf.sem_op = -1;
    block_trap_status_buf.sem_flg = 0;

    //Буфер для атомарной увеличения статуса trap
    struct sembuf unblock_trap_status_buf;
    unblock_trap_status_buf.sem_num = TRAP_STATUS;
    unblock_trap_status_buf.sem_op = 1;
    unblock_trap_status_buf.sem_flg = 0;

    struct sembuf unblock_break_buf;
    unblock_break_buf.sem_num = BREAK;
    unblock_break_buf.sem_op = -1;
    unblock_break_buf.sem_flg = 0;

    /* Изменение семафоров при поступлении нового человека с земли на трап
     */

    struct sembuf from_land_on_trap[3];
    from_land_on_trap[0].sem_num = ON_LAND;
    from_land_on_trap[0].sem_op = -1;
    from_land_on_trap[0].sem_flg = 0;
    from_land_on_trap[1].sem_num = BLOCK_LAND_FLOW;
    from_land_on_trap[1].sem_op = -1;
    from_land_on_trap[1].sem_flg = 0;
    from_land_on_trap[2].sem_num = BLOCK_BOAT_FLOW;
    from_land_on_trap[2].sem_op = 1;
    from_land_on_trap[2].sem_flg = 0;

    /* Изменение семафоров при поступлении нового человека с трапа на землю
     */

    struct sembuf from_trap_on_land[3];
    from_trap_on_land[0].sem_num = BLOCK_BOAT_FLOW;
    from_trap_on_land[0].sem_op = -1;
    from_trap_on_land[0].sem_flg = 0;
    from_trap_on_land[1].sem_num = BLOCK_LAND_FLOW;
    from_trap_on_land[1].sem_op = 1;
    from_trap_on_land[1].sem_flg = 0;
    from_trap_on_land[2].sem_num = ON_LAND;
    from_trap_on_land[2].sem_op = 1;
    from_trap_on_land[2].sem_flg = 0;
    /* Изменение семафоров при поступлении нового человека с трапа корабль
     */

    struct sembuf from_trap_on_boat[4];
    from_trap_on_boat[0].sem_num = BLOCK_BOAT_FLOW;
    from_trap_on_boat[0].sem_op = -1;
    from_trap_on_boat[0].sem_flg = 0;
    from_trap_on_boat[1].sem_num = SITS_LEFT;
    from_trap_on_boat[1].sem_op = -1;
    from_trap_on_boat[1].sem_flg = 0;
    from_trap_on_boat[2].sem_num = BLOCK_LAND_FLOW;
    from_trap_on_boat[2].sem_op = 1;
    from_trap_on_boat[2].sem_flg = 0;
    from_trap_on_boat[3].sem_num = ON_BOAT;
    from_trap_on_boat[3].sem_op = 1;
    from_trap_on_boat[3].sem_flg = 0;



    /* Изменение семафоров при поступлении нового человека с корабля на трап
     */

    struct sembuf from_boat_on_trap[4];
    from_boat_on_trap[0].sem_num = ON_BOAT;
    from_boat_on_trap[0].sem_op = -1;
    from_boat_on_trap[0].sem_flg = 0;
    from_boat_on_trap[1].sem_num = BLOCK_LAND_FLOW;
    from_boat_on_trap[1].sem_op = -1;
    from_boat_on_trap[1].sem_flg = 0;
    from_boat_on_trap[2].sem_num = BLOCK_BOAT_FLOW;
    from_boat_on_trap[2].sem_op = 1;
    from_boat_on_trap[2].sem_flg = 0;
    from_boat_on_trap[3].sem_num = SITS_LEFT;
    from_boat_on_trap[3].sem_op = 1;
    from_boat_on_trap[3].sem_flg = 0;

    /* Дождаться пока трап освободиться и закрыть процессы, отвечавше за освобождения трапа
     */

    struct sembuf wait_for_zero_and_break[2];
    wait_for_zero_and_break[0].sem_num = BLOCK_BOAT_FLOW;
    wait_for_zero_and_break[0].sem_op = 0;
    wait_for_zero_and_break[0].sem_flg = 0;
    wait_for_zero_and_break[1].sem_num = BREAK;
    wait_for_zero_and_break[1].sem_op = 1;
    wait_for_zero_and_break[1].sem_flg = 0;

    /* Дождаться пока трап скажет, что можно отплывать */
    struct sembuf wait_trap_status_block;
    wait_trap_status_block.sem_num = TRAP_STATUS;
    wait_trap_status_block.sem_op = 0;
    wait_trap_status_block.sem_flg = 0;

    struct sembuf wait_flow_block[2];
    wait_flow_block[0].sem_num = BLOCK_BOARDING;
    wait_flow_block[0].sem_op = 0;
    wait_flow_block[0].sem_flg = 0;
    wait_flow_block[1].sem_num = BLOCK_LANDING;
    wait_flow_block[1].sem_op = 0;
    wait_flow_block[1].sem_flg = 0;

    struct sembuf wait_flow_status_block[2];
    wait_flow_status_block[0].sem_num = BOARDING_STATUS;
    wait_flow_status_block[0].sem_op = 0;
    wait_flow_status_block[0].sem_flg = 0;
    wait_flow_status_block[1].sem_num = LANDING_STATUS;
    wait_flow_status_block[1].sem_op = 0;
    wait_flow_status_block[1].sem_flg = 0;

    int i = 0;
    int boat_flow;
    int land_flow;
    int on_boat;
    int on_land;
    int a = 0;
    int pid_1 = 0, pid_2 = 0;
    if (fork() == 0) {
        semop(sem_status, &block_boarding_status_buf, 1);
        while (1) {
            if (semop(sem_block, &block_boarding_buf, 1) == -1) {
                perror("semop on block_boarding");
                return 0;
            } else {
                printf("%d boarding blocked %d\n", a, semctl(sem_block, BLOCK_BOARDING, GETVAL));
            }

            if (semctl(sem_status, END_STATUS, GETVAL) == 1) {
                break;
            }

            printf("boat_begin %d. on_boat: %d, on_land: %d, land_flow: %d, boat_flow: %d\n", i,
                   semctl(sem_amount_block, ON_BOAT, GETVAL), semctl(sem_amount_block, ON_LAND, GETVAL),
                   semctl(sem_amount_block, BLOCK_LAND_FLOW, GETVAL),
                   semctl(sem_amount_block, BLOCK_BOAT_FLOW, GETVAL));

            pid_1 = fork();
            if (pid_1 == 0) {
                while (semctl(sem_status, BOARDING_STATUS, GETVAL) != 0) {
                    if (semop(sem_amount_block, from_trap_on_boat, 4) == -1) {
                        perror("semop on from_trap_on_boat:");
                        return 0;
                    }
                    i++;

                    // Если вдруг одно из условий науршится будет выведена соответсвующая запись

                    if (semctl(sem_amount_block, ON_BOAT, GETVAL) < 0 || semctl(sem_amount_block, ON_BOAT, GETVAL) > arg.capesity_of_ship){
                        printf("--board %d ship is full || ship is empty\n", i);
                    }
                    if (semctl(sem_amount_block, ON_LAND, GETVAL) < 0){
                        printf("--board %d land is empty\n", i);
                    }
                    if (semctl(sem_amount_block, BLOCK_BOAT_FLOW, GETVAL) < 0 || semctl(sem_amount_block, BLOCK_BOAT_FLOW, GETVAL) > arg.trap_val){
                        printf("--board %d trap is full || trap is empty\n", i);
                    }

                    //Напеатать всех пришедших и ушедших людей можно следующим образом:

                    /*printf("boat %d. on_boat: %d, on_land: %d, land_flow: %d, boat_flow: %d\n", i,
                           semctl(sem_amount_block, ON_BOAT, GETVAL),
                           semctl(sem_amount_block, ON_LAND, GETVAL),
                           semctl(sem_amount_block, BLOCK_LAND_FLOW, GETVAL),
                           semctl(sem_amount_block, BLOCK_BOAT_FLOW, GETVAL));*/
                }
                return 0;
            }
            pid_2 = fork();
            if (pid_2 == 0) {
                while (semctl(sem_status, BOARDING_STATUS, GETVAL) != 0) {
                    if (semop(sem_amount_block, from_boat_on_trap, 4) == -1) {
                        perror("semop on from_boat_on_trap");
                        return 0;
                    }
                    i++;

                    // Если вдруг одно из условий науршится будет выведена соответсвующая запись

                    if (semctl(sem_amount_block, ON_BOAT, GETVAL) < 0 || semctl(sem_amount_block, ON_BOAT, GETVAL) > arg.capesity_of_ship){
                        printf("--board %d ship is full || ship is empty\n", i);
                    }
                    if (semctl(sem_amount_block, ON_LAND, GETVAL) < 0){
                        printf("--board %d land is empty\n", i);
                    }
                    if (semctl(sem_amount_block, BLOCK_BOAT_FLOW, GETVAL) < 0 || semctl(sem_amount_block, BLOCK_BOAT_FLOW, GETVAL) > arg.trap_val){
                        printf("--board %d trap is full || trap is empty\n", i);
                    }

                    //Напеатать всех пришедших и ушедших людей можно следующим образом:

                    /*printf("boat %d. on_boat: %d, on_land: %d, land_flow: %d, boat_flow: %d\n", i,
                           semctl(sem_amount_block, ON_BOAT, GETVAL),
                           semctl(sem_amount_block, ON_LAND, GETVAL),
                           semctl(sem_amount_block, BLOCK_LAND_FLOW, GETVAL),
                           semctl(sem_amount_block, BLOCK_BOAT_FLOW, GETVAL));*/
                }
                return 0;
            }

            /* Дождемся пока высадка и посадка будет заблокирована трпаом и убьем процессы */

            semop(sem_status, wait_flow_status_block, 2);
            kill(pid_1, SIGKILL);
            kill(pid_2, SIGKILL);
            waitpid(0, NULL, WNOHANG);
            printf("%d iteration of boarding succeeded\n", a);
            a++;
        }
        printf("process on_boat ended successfully\n");
        return 0;
    }
    if (fork() == 0) {
        //on_land
        semop(sem_status, &block_landing_status_buf, 1);
        while (1) {
            if (semop(sem_block, &block_landing_buf, 1) == -1) {
                perror("semop on land block:");
                return 0;
            } else {
                printf("%d land_blocked %d\n", a, semctl(sem_block, BLOCK_LANDING, GETVAL));
            }

            if (semctl(sem_status, END_STATUS, GETVAL) == 1) {
                break;
            }

            printf("land_begin %d. on_boat: %d, on_land: %d, land_flow: %d, boat_flow: %d\n", i,
                   semctl(sem_amount_block, ON_BOAT, GETVAL), semctl(sem_amount_block, ON_LAND, GETVAL),
                   semctl(sem_amount_block, BLOCK_LAND_FLOW, GETVAL),
                   semctl(sem_amount_block, BLOCK_BOAT_FLOW, GETVAL));
            pid_1 = fork();
            if (pid_1 == 0) {
                while (semctl(sem_status, LANDING_STATUS, GETVAL) != 0) {
                    if (semop(sem_amount_block, from_trap_on_land, 3) == -1) {
                        perror("semop on from_trap_on_land:");
                        return 0;
                    }
                    i++;

                    // Если вдруг одно из условий науршится будет выведена соответсвующая запись

                    if (semctl(sem_amount_block, ON_BOAT, GETVAL) < 0 || semctl(sem_amount_block, ON_BOAT, GETVAL) > arg.capesity_of_ship){
                        printf("--land %d ship is full || ship is empty\n", i);
                    }
                    if (semctl(sem_amount_block, ON_LAND, GETVAL) < 0){
                        printf("--land %d land is empty\n", i);
                    }
                    if (semctl(sem_amount_block, BLOCK_BOAT_FLOW, GETVAL) < 0 || semctl(sem_amount_block, BLOCK_BOAT_FLOW, GETVAL) > arg.trap_val){
                        printf("--land %d trap is full || trap is empty\n", i);
                    }

                    //Напеатать всех пришедших и ушедших людей можно следующим образом:

                    /*printf("land %d. on_boat: %d, on_land: %d, land_flow: %d, boat_flow: %d\n", i,
                           semctl(sem_amount_block, ON_BOAT, GETVAL),
                           semctl(sem_amount_block, ON_LAND, GETVAL),
                           semctl(sem_amount_block, BLOCK_LAND_FLOW, GETVAL),
                           semctl(sem_amount_block, BLOCK_BOAT_FLOW, GETVAL));*/
                }
                return 0;
            }
            pid_2 = fork();
            if (pid_2 == 0) {
                while (semctl(sem_status, LANDING_STATUS, GETVAL) != 0) {
                    if (semop(sem_amount_block, from_land_on_trap, 3) == -1) {
                        perror("semop on from_land_on_trap:");
                        return 0;
                    }
                    i++;
                    // Если вдруг одно из условий науршится будет выведена соответсвующая запись
                    if (semctl(sem_amount_block, ON_BOAT, GETVAL) < 0 || semctl(sem_amount_block, ON_BOAT, GETVAL) > arg.capesity_of_ship){
                        printf("--land %d ship is full || ship is empty\n", i);
                    }
                    if (semctl(sem_amount_block, ON_LAND, GETVAL) < 0){
                        printf("--land %d land is empty\n", i);
                    }
                    if (semctl(sem_amount_block, BLOCK_BOAT_FLOW, GETVAL) < 0 || semctl(sem_amount_block, BLOCK_BOAT_FLOW, GETVAL) > arg.trap_val){
                        printf("--land %d trap is full || trap is empty\n", i);
                    }

                    //Напеатать всех пришедших и ушедших людей можно следующим образом:

                    /*printf("land %d. on_boat: %d, on_land: %d, land_flow: %d, boat_flow: %d\n", i,
                           semctl(sem_amount_block, ON_BOAT, GETVAL),
                           semctl(sem_amount_block, ON_LAND, GETVAL),
                           semctl(sem_amount_block, BLOCK_LAND_FLOW, GETVAL),
                           semctl(sem_amount_block, BLOCK_BOAT_FLOW, GETVAL));*/
                }
                return 0;
            }

            /* Дождемся пока высадка и посадка будет заблокирована трпаом и убьем процессы */

            semop(sem_status, wait_flow_status_block, 2);
            kill(pid_1, SIGKILL);
            kill(pid_2, SIGKILL);
            waitpid(0, NULL, WNOHANG);
            printf("%d iteration of landing succeeded\n", a);
            a++;
        }

        printf("process on_land ended successfully\n");
        return 0;
    }
    if (fork() == 0) {
        //on_trap
        while (1) {

            if (semop(sem_status, &block_trap_status_buf, 1) == -1) {
                perror("semop on trap status");
            } else {
                printf("trap_status blocked, value: %d\n", semctl(sem_status, TRAP_STATUS, GETVAL));
            }

            printf("%d trap blocking\n", a);

            if (semop(sem_block, &block_trap_buf, 1) == -1) {
                perror("semop on trap block");
            } else {
                printf("%d trap blocked\n", a);
            }

            /* Если от корабля пришло сообщение о том, что путешествие закончилось,
             * разблокируем посадку и высадку, чтобы процесс завершился */

            if (semctl(sem_status, END_STATUS, GETVAL) == 1) {
                if (semop(sem_block, unblock_flow_buf, 2) == -1) {
                    perror("semop on unblock_land_boat");
                } else {
                    printf("all flows terminated\n");
                }
                break;
            }

            /* Изменим статус посадки и высадки и разблокируем семафор, отвечавший за посадку и высадку */

            if (semop(sem_status, unblock_flow_status_buf, 2) == -1) {
                perror("semop on unblock_land_boat_status");
            } else {
                printf("%d land-boat unblocked_status\n", a);
            }

            if (semop(sem_block, unblock_flow_buf, 2) == -1) {
                perror("semop on unblock_land_boat");
            } else {
                printf("%d land-boat unblocked\n", a);
            }

            /* Посадка и высадка длится 1 секунду, в принципе можно выставить любое другое */

            sleep(1);

            /* Изменяме статус посадки и высадки */

            if (semop(sem_status, block_flow_status_buf, 2) == -1) {
                perror("semop on block_land_boat_status");
            } else {
                printf("%d land-boat blocked_status\n", a);
            }

            /* Дождемся, пока процессы высадки/посадки заблокируются, если вдруг они еще не успели этого сделать */

            semop(sem_block, wait_flow_block, 2);

            printf("%d attention, %d people are still on trap!!!\n", a, semctl(sem_amount_block, BLOCK_BOAT_FLOW, GETVAL));

            /* Всех оставшихся на трапе после окончания посадки отправляются на землю или на сушу,
             * при этом новых новых людей на трап не поступает */

            pid_1 = fork();
            if (pid_1 == 0) {
                while (semctl(sem_amount_block, BREAK, GETVAL) == 0) {
                    if (semop(sem_amount_block, from_trap_on_land, 3) == -1) {
                        printf("time is up on from_trap_on_land");
                        return 0;
                    }
                    printf("%d one went on land, %d remain\n", a, semctl(sem_amount_block, BLOCK_BOAT_FLOW, GETVAL));

                }
                return 0;
            }
            pid_2 = fork();
            if (pid_2 == 0) {
                while (semctl(sem_amount_block, BREAK, GETVAL) == 0) {
                    if (semop(sem_amount_block, from_trap_on_boat, 3) == -1) {
                        printf("time is up on from_trap_on_boat");
                        return 0;
                    }

                    printf("%d one went on boat, %d remain\n", a, semctl(sem_amount_block, BLOCK_BOAT_FLOW, GETVAL));
                }
                return 0;
            }

            /* Родительский процесс заблокирован до тех пор, пока количество людей на трапе не станет */

            semop(sem_amount_block, wait_for_zero_and_break, 2);

            /* Убъем процессы */

            kill(pid_1, SIGKILL);
            kill(pid_2, SIGKILL);
            waitpid(0, NULL, WNOHANG);

            /* Восстановим значение вспомогательного семафора BREAK */
            semop(sem_amount_block, &unblock_break_buf, 1);
            a++;
        }

        /* Экипаж прощяеся */

        printf("trap says goodbay\n");
        return 0;
    }

    if (fork() == 0) {
        //ship

        /* Перед тем как начать плавание, дождемся пока трап придет в начальное положение */

        semop(sem_status, &wait_trap_status_block, 1);

        printf("start\n");

        for (i = 0; i < arg.trip_val; i++) {

            /* Как только корабль причалил, изменим статус трапа и разблокируем семафор блокирующий трап*/

            if (semop(sem_status, &unblock_trap_status_buf, 1) == -1) {
                perror("semop on unlocking trap_status");
            } else {
                printf("%d trap status unblocked\n", i);
            }

            printf("%d trap_block %d\n", i, semctl(sem_block, TRAP_STATUS, GETVAL));

            if (semop(sem_block, &unblock_trap_buf, 1) == -1) {
                perror("semop on unlocking trap");
            } else {
                printf("%d trap unlocked, blocking ship, trap_block %d\n", i, semctl(sem_block, BLOCK_TRAP, GETVAL));
            }

            /* Дождемся с трапа сигнала о том, что корабль может отплывать, то есть додемся блокировки трапа*/

            semop(sem_status, &wait_trap_status_block, 1);

            /* Корабль отправляется в 2-ух секундное плавание, в принципе можно любое выставлять */

            sleep(2);
        }

        if (semop(sem_status, &unblock_end_buf, 1) == -1) {
            perror("semop on end");
        } else {
            printf("%d end unlocked\n", i);
        }

        if (semop(sem_status, &unblock_trap_status_buf, 1) == -1) {
            perror("semop on unlocking trap_sem");
        } else {
            printf("%d last_trap_status unblocked\n", i);
        }

        if (semop(sem_block, &unblock_trap_buf, 1) == -1) {
            perror("semop on unlocking trap");
        } else {
            printf("%d last_trap unblock\n", i);
        }
        printf("ship says goodbay\n");
        return 0;
    }

    wait(NULL);
    wait(NULL);
    wait(NULL);
    wait(NULL);

    semctl(sem_amount_block, 0, IPC_RMID, NULL);
    semctl(sem_block, 0, IPC_RMID, NULL);
    semctl(sem_status, 0, IPC_RMID, NULL);
    return 0;
}
