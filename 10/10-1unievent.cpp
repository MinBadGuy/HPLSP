#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <pthread.h>

#define MAX_EVENT_NUMBER 1024
static int pipefd[2];   // 管道

/**
 * @brief: 将文件描述符fd设置成非阻塞的
 * @param fd: 文件描述符fd
 * @return: 文件描述符fd原来的状态标志
*/
int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

/**
 * @brief: 将文件描述fd上的EPOLLIN和EPOLLET事件注册到epollfd指示的epoll内核事件表中
 * @param epollfd: 内核事件表
 * @param fd: 文件描述符
 * @return: 
*/
void addfd(int epollfd, int fd)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

/**
 * @brief: 信号处理函数
 * @param sig: 信号
 * @return:
*/
void sig_handler(int sig)
{
    // 保留原来的errno，在函数最后恢复，以保证函数的可重入性
    int save_errno = errno;
    int msg = sig;
    send(pipefd[1], (char*)&msg, 1, 0); // 将信号值写入管道，以通知主循环
    errno = save_errno;
}

/**
 * @brief: 设置信号的处理函数
 * @param sig: 信号
 * @return:
*/
void addsig(int sig)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = sig_handler;
    sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

int main(int argc, char* argv[])
{
    if (argc <= 2)
    {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);

    int ret = 0;
    // socket地址
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    // 1. 创建监听socket
    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);

    // 2. 将监听socket绑定至具体的地址
    ret = bind(listenfd, (const sockaddr*)&address, sizeof(address));
    if (ret == -1)
    {
        printf("errno is %d\n", errno);
        return 1;
    }
    
    // 3. 监听socket
    ret = listen(listenfd, 5);
    assert(ret != -1);

    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);  // 内核事件表文件描述符
    assert(epollfd != -1);
    addfd(epollfd, listenfd);   // 将监听socket注册至内核事件表
#if 0
    // 使用socketpair创建管道，注册pipefd[0]上的可读事件
    ret = socketpair(PF_INET, SOCK_STREAM, 0, pipefd);  // ？一直返回-1
#endif
    ret = pipe(pipefd);
    // printf("ret = %d\n", ret);
    assert(ret != -1);

    setnonblocking(pipefd[1]);
    addfd(epollfd, pipefd[0]);

    // 设置一些信号处理函数 p179
    addsig(SIGHUP);
    addsig(SIGCHLD);
    addsig(SIGTERM);
    addsig(SIGINT);
    bool stop_server = false;

    while (!stop_server)
    {
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if ((number < 0) && (errno != EINTR))
        {
            printf("epoll failure\n");
            break;
        }

        for (int i = 0; i < number; i++)
        {
            int sockfd = events[i].data.fd;
            // 如果就绪的文件描述符是listenfd，则处理新的连接
            if (sockfd == listenfd)
            {
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof(client_address);
                int connfd = accept(listenfd, (sockaddr*)&client_address, &client_addrlength);
                addfd(epollfd, connfd);
            }
            // 如果就绪的文件描述符是pipefd[0]，则处理信号
            else if ((sockfd == pipefd[0]) && (events[i].events & EPOLLIN))
            {
                int sig;
                char signals[1024];
                ret = recv(pipefd[0], signals, sizeof(signals), 0);
                if (ret == -1)
                {
                    continue;
                }
                else if (ret == 0)
                {
                    continue;
                }
                else
                {
                    // 因为每个信号值占1字节，所以按字节来逐个接收信号。以SIGTERM为例，来说明如何安全地终止服务器主循环
                    for (int i = 0; i < ret; i++)
                    {
                        switch (signals[i])
                        {
                            case SIGCHLD:
                            case SIGHUP:
                            {
                                continue;
                            }
                            case SIGTERM:
                            case SIGINT:
                            {
                                stop_server = true; // 测试现象：输入ctrl+c后未结束，输入ctrl+z后退出了，但ps发现进程还在
                            }
                        }
                    }    
                }
            }
            else
            {
                
            }
        }
    }
    
    printf("close fds\n");
    close(listenfd);
    close(pipefd[1]);
    close(pipefd[0]);
    return 0;
}