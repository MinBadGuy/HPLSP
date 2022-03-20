#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

int main(int argc, char* argv[])
{
    if (argc <= 2)
    {
       printf("usage: %s ip_address port_number\n", basename(argv[0]));
       return 1;
    }

    const char* ip = argv[1];
    int port = atoi(argv[2]);

    // 服务端socket地址
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    // 服务端scoket
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

    // 将socket与socket地址绑定
    int ret = bind(sock, (const sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    // 监听socket
    ret = listen(sock, 5);      // sock：监听socket
    assert(ret != -1);

    /* 暂停20秒以等待客户端连接和相关操作（掉线或退出）完成 */
    sleep(20);
    
    // 客户端socket地址
    struct sockaddr_in client;
    socklen_t client_addrlength = sizeof(client);

    // 服务端被动连接
    int connfd = accept(sock, (struct sockaddr*)&client, &client_addrlength);   // connfd：连接socket
    if (connfd < 0)
    {
        printf("errno is: %d\n", errno);
    }
    else
    {
        /* 接受连接成功则打印出客户端的IP地址和端口号 */
        char remote[INET_ADDRSTRLEN];
        printf("connected with ip: %s and port: %d\n",\
                inet_ntop(AF_INET, &client.sin_addr, remote, INET_ADDRSTRLEN),\
                ntohs(client.sin_port));    // ntohs: 将一个无符号短整型数从网络字节序转换为主机字节序，即小端模式(little-endian)
        // sleep(30);
        close(connfd);
    }

    close(sock);
    return 0;
}