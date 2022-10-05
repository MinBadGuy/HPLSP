#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define BUFFER_SIZE 1024

int main(int argc, char* argv[])
{
    if (argc <= 3)
    {
        printf("usage: %s ip_address port_number recv_buffer_size\n", basename(argv[0]));
        return 1;
    }

    const char* ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    // 接收端socket
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

    int recvbuf = atoi(argv[3]);
    int len = sizeof(recvbuf);
    /* 先设置TCP接收缓存区的大小，然后读取之 */
    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &recvbuf, sizeof(recvbuf));
    getsockopt(sock, SOL_SOCKET, SO_RCVBUF, &recvbuf, (socklen_t*)&len);
    printf("the tcp receive buffer size after setting is %d\n", recvbuf);

    // 接收端socket绑定具体的socket地址
    int ret = bind(sock, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    // 接收端socket开启监听
    ret = listen(sock, 5);
    assert(ret != -1);

    struct sockaddr_in client;
    socklen_t client_addrlength = sizeof(client);
    int connfd = accept(sock, (struct sockaddr*)&client, &client_addrlength);   // 被动连接
    if (connfd < 0)
    {
        printf("errno is: %d\n", errno);
    }
    else
    {
        char buffer[BUFFER_SIZE];
        memset(buffer, '\0', BUFFER_SIZE);
        while (recv(sock, buffer, BUFFER_SIZE - 1, 0))
        {
            /* code */
        }
        close(connfd);
    }

    close(sock);

    return 0;
}