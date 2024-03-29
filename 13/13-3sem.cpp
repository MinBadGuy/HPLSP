#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

union semun
{
    int val;
    struct semid_ds* buf;
    unsigned short int* array;
    struct seminfo* __buf;
};

/* op为-1时执行P操作，op为1时执行V操作 */
void pv(int sem_id, int op)
{
    struct sembuf sem_b;
    sem_b.sem_num = 0;
    sem_b.sem_op = op;
    sem_b.sem_flg = SEM_UNDO;   // SEM_UNDO含义：当进程退出时取消正在进行的semop操作
    semop(sem_id, &sem_b, op);
}

int main(int argc, char* argv[])
{
    int sem_id = semget(IPC_PRIVATE, 1, 0666);  // IPC_PRIVATE：创建一个新的信号量集

    union semun sem_un;
    sem_un.val = 1;
    semctl(sem_id, 0, SETVAL, sem_un);  // 设置信号量集sem_id中编号为0的信号量的值

    pid_t id = fork();
    if (id < 0)
    {
        return 1;
    }
    else if (id == 0)   // 子进程
    {
        printf("child try to get binary sem\n");
        /* 在父、子进程间共享IPC_PRIVATE信号量的关键就在于二者都可以操作该信号量的标志符sem_id */
        pv(sem_id, -1);
        printf("child get the sem and would release it after 5 seconds\n");
        sleep(5);
        pv(sem_id, 1);
        exit(0);
    }
    else    // 父进程
    {
        printf("parent try to get binary sem\n");
        pv(sem_id, -1);
        printf("parent get the sem and would release it after 5 seconds\n");
        sleep(5);
        pv(sem_id, 1);
    }
    
    printf("wait child finish\n");
    waitpid(id, NULL, 0);   // 等待子进程运行结束
    printf("child finished\n");
    semctl(sem_id, 0, IPC_RMID, sem_un);    // 删除信号量
    return 0;
}