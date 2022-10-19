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
#define BUFFER_SIZE 10

/**
 * @brief: 将文件描述符fd设置成非阻塞的
 * @param fd: 文件描述符fd
 * @return: 文件描述符fd原来的状态标志
*/
int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);    // 获取文件描述符当前状态标志
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

/**
 * @brief: 将文件描述fd上的EPOLLIN注册到epollfd指示的epoll内核事件表中
 * @param epollfd: 内核事件表
 * @param fd: 文件描述符
 * @param enable_et: 指定是否对fd启用ET模式
 * @return: 
*/
void addfd(int epollfd, int fd, bool enable_et)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN;
    if (enable_et)
    {
        event.events |= EPOLLET;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);  // 往内核事件表中注册事件
    setnonblocking(fd);
}


/**
 * @brief: LT模式的工作流程
 * @param events: 事件数组
 * @param number: 数组长度
 * @param epollfd: 内核事件表
 * @param listenfd: 监听的socket
 * @return:
*/
void lt(epoll_event* events, int number, int epollfd, int listenfd)
{
    char buf[BUFFER_SIZE];
    for (int i = 0; i < number; i++)
    {
        int sockfd = events[i].data.fd;
        if (sockfd == listenfd)
        {
            struct sockaddr_in client_address;
            socklen_t client_addrlength = sizeof(client_address);
            int connfd = accept(sockfd, (struct sockaddr*)&client_address, &client_addrlength);
            addfd(epollfd, sockfd, false);  // 对connfd禁用ET模式
        }
        else if (events[i].events & EPOLLIN)
        {
            /* 只要socket读缓存中还有未读出的数据，这段代码就被触发 */
            printf("event trigger once\n");
            memset(buf, '\0', BUFFER_SIZE);
            int ret = recv(sockfd, buf, BUFFER_SIZE - 1, 0);
            if (ret <= 0)
            {
                close(sockfd);
                continue;
            }
            printf("get %d bytes of content: %s\n", ret, buf);
        }
        else
        {
            printf("something else happened\n");
        }        
    }
}

/**
 * @brief: LT模式的工作流程
 * @param events: 事件数组
 * @param number: 数组长度
 * @param epollfd: 内核事件表
 * @param listenfd: 监听的socket
 * @return:
*/
void et(epoll_event* events, int number, int epollfd, int listenfd)
{
    char buf[BUFFER_SIZE];
    for (int i = 0; i < number; i++)
    {
        int sockfd = events[i].data.fd;
        if (sockfd == listenfd)
        {
            struct sockaddr_in client_address;
            socklen_t client_addrlength = sizeof(client_address);
            int connfd = accept(sockfd, (struct sockaddr*)&client_address, &client_addrlength);
            addfd(epollfd, connfd, true);   // 对connfd开启ET模式
        }
        else if (events[i].events & EPOLLIN)
        {
            /* 这段代码不会被重复触发，所以循环读取数据，以确保把socket读缓存中的所有数据读出 */
            printf("event trigger once\n");
            while (1)
            {
                memset(buf, '\0', BUFFER_SIZE);
                int ret = recv(sockfd, buf, BUFFER_SIZE - 1, 0);
                if (ret < 0)
                {   
                    /* 对于非阻塞IO，下面的条件成立表示数据已经全部读取完毕。
                    此后，epoll就能再次触发sockfd上的EPOLLIN事件，以驱动下一次读操作 */
                    if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
                    {
                        printf("read later\n");
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
                    printf("get %d bytes of content: %s\n", ret, buf);
                }
            }   
        }
        else
        {
            printf("something else happened\n");
        }
    }
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

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);

    ret = bind(listenfd, (sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(listenfd, 5);
    assert(ret != -1);

    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);  // 内核事件表
    assert(epollfd != -1);
    addfd(epollfd, listenfd, true);

    while (1)
    {
        int ret = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if (ret < 0)
        {
            printf("epoll failure\n");
            break;
        }
        
        lt(events, ret, epollfd, listenfd);     // 使用LT模式
        // et(events, ret, epollfd, listenfd);     // 使用ET模式
    }
    close(listenfd);
    return 0;
}