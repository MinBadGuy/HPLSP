#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

/* 信号处理函数 */
void sig_handler(int sig)
{
    int save_errno = errno;
    printf("start process signal: %d\n", sig);
    sleep(5);
    printf("end process\n");
    errno = save_errno;
}

/* 为信号sig设置它的处理函数 */
void addsig(int sig)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = sig_handler;
    sa.sa_flags |= SA_RESTART;
    /*
        sigfillset(&sa.sa_mask)的作用是：在信号处理函数执行过程中，屏蔽所有信号
        （1）如果执行过程中，接收到其他信号，则把它先挂起，等此处理函数执行完毕再去处理接收到的信号
        （2）如果执行过程中多次接收到同一个信号，处理函数执行完毕后只去处理接收到的信号一次
        可在本程序代码触发一次的sleep的5s期间验证这两点
    */ 
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

int main()
{
    addsig(SIGINT); // 2
    char ch;
    while (ch != 'q')
    {
        printf("press 'Ctrl+C' to generate a SIGINT and 'q' to exit\n");
        scanf("%c", &ch);
    }
    
    return 0;
}