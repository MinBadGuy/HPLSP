#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

int main(int argc, char* argv[])
{
    if ((argc <= 2))
    {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }

    const char* ip = argv[1];
    int port = atoi(argv[2]);

    // socket地址
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    // 创建socket
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);
    
    // socket绑定具体地址
    int ret = bind(sock, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    // 监听socket
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
        int pipefd[2];
        ret = pipe(pipefd);     /* 创建管道，pipefd[1]写入，pipefd[0]读取 */
        assert(ret != -1);
        
        /* 将connfd上流入的客户数据定向到管道中 connfd->pipefd[1] */
        ret = splice(connfd, NULL, pipefd[1], NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);  // #include <fcntl.h>
        assert(ret != -1);
        
        /* 将管道的输出定向到connfd客户连接文件描述符 pipefd[0]->connfd */
        ret = splice(pipefd[0], NULL, connfd, NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
        assert(ret != -1);

        close(connfd);
    }

    close(sock);
    return 0;
}