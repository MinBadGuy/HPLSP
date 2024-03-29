#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>

int main(int argc, char* argv[])
{
    assert(argc == 2);
    char* host = argv[1];

    /* 获取目标主机地址信息 */
    struct hostent* hostinfo = gethostbyname(host);
    assert(hostinfo);

    /* 获取daytime服务信息 */
    struct servent* servinfo = getservbyname("daytime", "tcp");
    assert(servinfo);
    printf("daytime port is %d\n", ntohs(servinfo->s_port));

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = servinfo->s_port;
    /* 因为h_addr_list本身是使用网络字节序的地址列表，所以使用其中的IP地址时，无需对目标IP地址转换字节序 */
    address.sin_addr = *(in_addr*)*(hostinfo->h_addr_list);

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    int result = connect(sockfd, (struct sockaddr*)&address, sizeof(address));  // 主动连接
    assert(result != -1);

    char buffer[128];
    result = read(sockfd, buffer, sizeof(buffer));
    assert(result > 0);
    buffer[result] = '\0';
    printf("the day time is: %s", buffer);
    
    close(sockfd);
    return 0;
}


/**
 * 执行该程序时需要开启daytime服务，
 * 具体参考：https://www.likecs.com/show-204100192.html
*/