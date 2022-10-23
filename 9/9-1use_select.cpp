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

int main(int argc, char* argv[])
{
    if (argc <= 2)
    {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return -1;
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

    // 1. 创建socket
    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);

    // 2. 绑定socket地址
    ret = bind(listenfd, (sockaddr *)&address, sizeof(address));
    assert(ret != -1);

    // 3. 监听socket
    ret = listen(listenfd, 5);
    assert(ret != -1);

    // 用于accept中存储客户端socket地址
    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof(client_address);

    // 4. 被动接受连接
    int connfd = accept(listenfd, (sockaddr*)&client_address, &client_addrlength);
    if (connfd < 0)
    {
        printf("errno is: %d\n", errno);
        close(listenfd);
    }
    
    char buf[1024];
    // 定义可读、异常文件描述符集合
    // fd_set结构里有个数组fds_bits，数组每个元素的每一位标记一个文件描述符
    fd_set read_fds;
    /*
    printf("__NFDBITS: %d\n", __NFDBITS);           // 64
    printf("__FD_SETSIZE: %d\n", __FD_SETSIZE);     // 1024
    printf("__fd_mask: %d\n", sizeof(__fd_mask));   // 8
    printf("fds_bits size: %d\n", sizeof(read_fds.fds_bits));   // 128  （数组长度*每个元素大小）16*8
    */
    fd_set exception_fds;
    FD_ZERO(&read_fds);     // 清除文件描述符集合的所有位
    FD_ZERO(&exception_fds);

    while (1)
    {
        memset(buf, '\0', sizeof(buf));
        /**
         * 每次调用select前都要重新在read_fds和exception_fds中设置文件描述符connfd，
         * 因为事件发生之后，文件描述符集合将被内核修改 */
        FD_SET(connfd, &read_fds);  // 设置文件描述符集合的第connfd位
        FD_SET(connfd, &exception_fds);
        ret = select(connfd + 1, &read_fds, NULL, &exception_fds, NULL);    // timeval*设置为NULL,select将一直阻塞，一直到某个文件描述符就绪
        if (ret < 0)
        {
            printf("selection failure\n");
            break;
        }
        
        /* 对于可读事件，采用普通的recv函数读取数据 */
        if (FD_ISSET(connfd, &read_fds))
        {
            ret = recv(connfd, buf, sizeof(buf) - 1, 0);
            if (ret <= 0)
            {
                break;
            }
            printf("get %d bytes of normal data: %s\n", ret, buf);  // ret为什么比实际输入字符数大2？因为回车？   例如：输入abcdefg，ret返回9
        }
        /* 对于异常事件，采用MSG_OOB标志的recv函数读取带外数据 */
        else if (FD_ISSET(connfd, &exception_fds))
        {
            ret = recv(connfd, buf, sizeof(buf) - 1, MSG_OOB);
            if (ret <= 0)
            {
                break;
            }
            printf("get %d bytes of oob data: %s\n", ret, buf);
        }
        
        close(connfd);
        close(listenfd);
        return 0;
    }
}