/* Parent/child bank account simulation using System V shared memory. */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

#define ITERATIONS 25
#define BANK_ACCOUNT 0
#define TURN 1

static void dad_process(volatile int *shared) {
    int i;
    srand((unsigned int)(time(NULL) ^ getpid()));
    for (i = 0; i < ITERATIONS; ++i) {
        int account, balance;
        sleep((unsigned int)(rand() % 6));
        account = shared[BANK_ACCOUNT];
        while (shared[TURN] != 0) { }
        if (account <= 100) {
            balance = rand() % 101;
            if (balance % 2 == 0) {
                account += balance;
                printf("Dear old Dad: Deposits $%d / Balance = $%d
",
                       balance, account);
            } else {
                printf("Dear old Dad: Doesn't have any money to give
");
            }
        } else {
            printf("Dear old Dad: Thinks Student has enough Cash ($%d)
",
                   account);
        }
        shared[BANK_ACCOUNT] = account;
        shared[TURN] = 1;
        fflush(stdout);
    }
}

static void student_process(volatile int *shared) {
    int i;
    srand((unsigned int)(time(NULL) ^ getpid()));
    for (i = 0; i < ITERATIONS; ++i) {
        int account, balance;
        sleep((unsigned int)(rand() % 6));
        account = shared[BANK_ACCOUNT];
        while (shared[TURN] != 1) { }
        balance = rand() % 51;
        printf("Poor Student needs $%d
", balance);
        if (balance <= account) {
            account -= balance;
            printf("Poor Student: Withdraws $%d / Balance = $%d
",
                   balance, account);
        } else {
            printf("Poor Student: Not Enough Cash ($%d)
", account);
        }
        shared[BANK_ACCOUNT] = account;
        shared[TURN] = 0;
        fflush(stdout);
    }
}

int main(void) {
    int shmid, status;
    volatile int *shared;
    pid_t child;

    shmid = shmget(IPC_PRIVATE, 2 * sizeof(int), IPC_CREAT | 0600);
    if (shmid == -1) {
        perror("shmget");
        return EXIT_FAILURE;
    }
    shared = (volatile int *)shmat(shmid, NULL, 0);
    if (shared == (void *)-1) {
        perror("shmat");
        shmctl(shmid, IPC_RMID, NULL);
        return EXIT_FAILURE;
    }

    shared[BANK_ACCOUNT] = 0;
    shared[TURN] = 0;
    child = fork();
    if (child < 0) {
        perror("fork");
        shmdt((const void *)shared);
        shmctl(shmid, IPC_RMID, NULL);
        return EXIT_FAILURE;
    }
    if (child == 0) {
        student_process(shared);
        if (shmdt((const void *)shared) == -1) {
            perror("shmdt");
            _exit(EXIT_FAILURE);
        }
        _exit(EXIT_SUCCESS);
    }

    dad_process(shared);
    if (waitpid(child, &status, 0) == -1) {
        perror("waitpid");
        shmdt((const void *)shared);
        shmctl(shmid, IPC_RMID, NULL);
        return EXIT_FAILURE;
    }
    if (shmdt((const void *)shared) == -1) {
        perror("shmdt");
        shmctl(shmid, IPC_RMID, NULL);
        return EXIT_FAILURE;
    }
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl");
        return EXIT_FAILURE;
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS) {
        fprintf(stderr, "Child process failed.
");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
