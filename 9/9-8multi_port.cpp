#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <pthread.h>

#define MAX_EVENT_NUMBER 1024
#define TCP_BUFFER_SIZE 512
#define UDP_BUFFER_SIZE 1024

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
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    /* 创建TCP socket，并将其绑定到端口port上 */
    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd > 0);

    ret = bind(listenfd, (sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(listenfd, 5);
    assert(ret != -1);

    /* 创建UDP socket 并将其绑定到端口port上 */
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);             // UDP与TCP使用相同的ip和端口号
    int udpfd = socket(PF_INET, SOCK_DGRAM, 0);
    assert(udpfd > 0);

    ret = bind(udpfd, (sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);
    assert(epollfd != -1);
    /* 注册TCP socket和UDP socket上的可读事件 */
    addfd(epollfd, listenfd);
    addfd(epollfd, udpfd);

    while (1)
    {
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if (number < 0)
        {
            printf("epoll failure\n");
            break;
        }

        for (int i = 0; i < number; i++)
        {
            int sockfd = events[i].data.fd;
            if (sockfd == listenfd) // listenfd
            {
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof(client_address);
                int connfd = accept(listenfd, (sockaddr*)&client_address, &client_addrlength);
                addfd(epollfd, connfd);
            }
            else if (sockfd == udpfd)   // udpfd
            {
                printf("UDP packet\n");
                char buf[UDP_BUFFER_SIZE];
                memset(buf, '\0', UDP_BUFFER_SIZE);
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof(client_address);
                /* 接收并回发数据 */
                ret = recvfrom(udpfd, buf, UDP_BUFFER_SIZE - 1, 0, (sockaddr*)&client_address, &client_addrlength);
                if (ret > 0)
                {
                    sendto(udpfd, buf, UDP_BUFFER_SIZE - 1, 0, (sockaddr*)&client_address, client_addrlength);
                }
            }
            else if (events[i].events & EPOLLIN)    // TCP socket，即110行注册的socket
            {
                printf("TCP packet\n");
                char buf[TCP_BUFFER_SIZE];
                while (1)
                {
                    memset(buf, '\0', TCP_BUFFER_SIZE);
                    ret = recv(sockfd, buf, TCP_BUFFER_SIZE - 1, 0);
                    if (ret < 0)
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                        {
                            break;
                        }
                        close(sockfd);
                        break;
                    }
                    else if (ret == 0)
                    {
                        close(sockfd);
                    }
                    else
                    {
                        send(sockfd, buf, ret, 0);
                    }
                }
            }
            else
            {
                printf("something else happened\n");
            }
        }
    }
    
    close(listenfd);
    return 0;
}