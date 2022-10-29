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
    printf("get signal: %d\n", sig);
    errno = save_errno;
}

/* 为信号sig设置它的处理函数 */
void addsig(int sig)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = sig_handler;
    sa.sa_flags |= SA_RESTART;
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