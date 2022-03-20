#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

static bool stop = false;

/* SIGTERM信号的处理函数，触发时结束主程序中的循环 */
static void handle_term(int sig)
{
    stop = true;
}

int main(int argc, char* argv[])
{
    signal(SIGTERM, handle_term);

    if (argc <= 3)
    {
        printf("usage: %s ip_address port_number backlog\n", basename(argv[0]));
        return 1;
    }
    
    const char* ip = argv[1];
    int port = atoi(argv[2]);                       // Convert a string to an integer
    int backlog = atoi(argv[3]);

    /* 创建一个IPv4 TCP套接字 */
    int sock = socket(PF_INET, SOCK_STREAM, 0);     // 协议族：PF_INET，流服务：SOCK_STREAM
    assert(sock >= 0);

    /* 创建一个IPv4 socket地址 */
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;                   // 地址族：AF_INET
    inet_pton(AF_INET, ip, &address.sin_addr);      // IP地址转换：点分十进制字符串IP地址转成网络字节序整数格式
    address.sin_port = htons(port);                 // htons：将一个无符号短整型数值从主机字节序转换为网络字节序，即大端模式(big-endian)

    /* 将socket与socket地址绑定 */
    int ret = bind(sock, (const sockaddr*)&address, sizeof(address));   // 强制转换成通用的socket地址类型sockaddr
    assert(ret != -1);

    /* 监听socket */
    ret = listen(sock, 5);
    assert(ret != -1);

    /* 循环等待连接，直到有SIGTERM信号将它中断 */
    while (!stop)
    {
       sleep(1);
    }

    /* 关闭socket */
    close(sock);
    return 0;
}