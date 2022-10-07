#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#define BUFFER_SIZE 4096	// 读缓冲区大小 

/* 主状态机的两种可能状态 */
enum CHECK_STATE { 
	CHECK_STATE_REQUESTLINE = 0,	// 当前正在分析请求行
	CHECK_STATE_HEADER				// 当前正在分析头部字段
};

/* 从状态机的三种可能状态，即行的读取状态 */
enum LINE_STATUS { 
	LINE_OK = 0,	// 读取到一个完整的行
	LINE_BAD,		// 行出错
	LINE_OPEN		// 行数据尚且不完整
};

/* 服务器处理HTTP请求的结果 */
enum HTTP_CODE { 
	NO_REQUEST, 		// 请求不完整，需要继续读取客户数据
	GET_REQUEST, 		// 获得了一个完整的客户请求
	BAD_REQUEST, 		// 客户请求有语法错误
	FORBIDDEN_REQUEST,	// 客户对资源没有足够的访问权限
	INTERNAL_ERROR, 	// 服务器内部错误
	CLOSED_CONNECTION	// 客户端已经关闭连接
};

/* HTTP应答报文 */
static const char* szret[] = { "I get a correct result\n", "Something wrong\n" };

/**
 * @brief: 从状态机，用于解析出一行内容
 * @param buffer: 应用程序的读缓冲区
 * @param checked_index: 指向buffer中当前正在分析的字节
 * @param read_index: 指向buffer中客户数据尾部的下一字节
 * @return: LINE_STATUS类型状态
*/
LINE_STATUS parse_line(char* buffer, int& checked_index, int& read_index)
{
	char temp;

	for (; checked_index < read_index; checked_index++)
	{
		temp = buffer[checked_index];	// 当前要分析的字节
		if (temp == '\r')	// 如果当前的字节是'\r'，即回车符，则说明可能读取到一个完整的行
		{
			if (checked_index + 1 == read_index)	// 如果'\r'字符是目前buffer中的最后一个被读入的数据，说明这次分析没有读入一个完整的行
			{
				return LINE_OPEN;
			}
			else if (buffer[checked_index + 1] == '\n')	// 如果下一个字符是'\n'，说明这次读取到一个完整的行
			{
				buffer[checked_index++] = '\0';
				buffer[checked_index++] = '\0';
				return LINE_OK;
			}
			return LINE_BAD;
		}
		else if (temp == '\n')	// 如果下一个字符是'\n'，说明这次可能读取到一个完整的行
		{
			if ((checked_index > 1) && buffer[checked_index - 1] == '\r')
			{
				buffer[checked_index-1] = '\0';
				buffer[checked_index++] = '\0';
				return LINE_OK;
			}
			return LINE_BAD;
		}
	}
	// 如果所有内容都分析完毕也没遇到'\r'字符，表示还需要继续读取客户数据
	return LINE_OPEN;
}

/**
 * @brief: 分析请求行
 * @param temp: http请求行
 * @param checkstate: 当前主状态机的状态
 * @return: HTTP_CODE类型状态
*/
HTTP_CODE parse_requestline(char* temp, CHECK_STATE& checkstate)
{
	char* url = strpbrk(temp, " \t");	// 检索字符串temp中第一个匹配字符串" \t"中字符的字符
	if (!url)	// 如果请求行中没有空白字符或者'\t'字符，则请求必有问题 
	{
		return BAD_REQUEST;
	}
	*url++ = '\0';

	char* method = temp;
	// 忽略大小写进行字符串比较
	if (strcasecmp(method, "GET") == 0)	// 仅支持GET方法
	{
		printf("The request method is GET\n");
	}
	else
	{
		return BAD_REQUEST;
	}
	
	url += strspn(url, " \t");	// 检索字符串url中第一个不在字符串" \t"中出现的字符下标
	char* version = strpbrk(url, " \t");
	if (!version)
	{
		return BAD_REQUEST;
	}
	*version++ = '\0';
	version += strspn(version, " \t");
	if (strcasecmp(version, "HTTP/1.1") != 0)	// 仅支持HTTP/1.1
	{
		return BAD_REQUEST;
	}

	// 忽略大小写，将url前7个字符和 "http://" 比较
	if (strncasecmp(url, "http://", 7) == 0)	// 检查url是否合法
	{
		url += 7;
		url = strchr(url, '/');	// 指向字符串url中字符'/'第一次出现的位置
	}

	if (!url || url[0] != '/')
	{
		return BAD_REQUEST;
	}
	printf("The request URL is %s\n", url);
	
	// HTTP请求行处理完毕，状态转移到头部字段的分析
	checkstate = CHECK_STATE_HEADER;

	return NO_REQUEST;
}

/**
 * @brief: 分析头部字段
 * @param temp: http请求头
 * @return: HTTP_CODE
*/
HTTP_CODE parse_headers(char* temp)
{
	// 遇到一个空行，说明得到了一个正确的http请求
	if (temp[0] == '\0')
	{
		return GET_REQUEST;
	}
	else if (strncasecmp(temp, "Host:", 5) == 0)	// 处理"Host"头部字段
	{
		temp += 5;
		temp += strspn(temp, " \t");
		printf("the request host is: %s\n", temp);
	}
	else	// 其他头部字段都不处理
	{	
		printf("I can not handle this header\n");
	}
	
	return NO_REQUEST;
}

/**
 * @brief: 分析HTTP请求的入口函数
 * @param buffer: 应用程序的读缓冲区
 * @param checked_index: 指向buffer中当前正在分析的字节
 * @param checkstate: 当前主状态机的状态
 * @param read_index: 指向buffer中客户数据尾部的下一字节
 * @param start_line: 行在buffer中的起始位置
 * @return:
*/
HTTP_CODE parse_content(char* buffer, int& checked_index, CHECK_STATE& checkstate, int& read_index, int& start_line)
{
	LINE_STATUS linestatus = LINE_OK;	// 记录当前行的读取状态
	HTTP_CODE retcode = NO_REQUEST;		// 记录http请求的处理结果

	// 主状态机，用于从buffer中取出所有完整的行
	while ((linestatus = parse_line(buffer, checked_index, read_index)) == LINE_OK)
	{
		char* temp = buffer + start_line;
		printf("temp: %s\n", temp);
		start_line = checked_index;	// 记录下一行的起始位置

		switch (checkstate)
		{
		case CHECK_STATE_REQUESTLINE:
			retcode = parse_requestline(temp, checkstate);
			if (retcode == BAD_REQUEST)
			{
				return BAD_REQUEST;
			}
			break;
		case CHECK_STATE_HEADER:
			retcode = parse_headers(temp);
			if (retcode == BAD_REQUEST)
			{
				return BAD_REQUEST;
			}
			else if (retcode == GET_REQUEST)
			{
				return GET_REQUEST;
			}
			break;
		default:
			return INTERNAL_ERROR;
		}
	}

	if (linestatus == LINE_OPEN)	// 没有读取到一个完整的行，需要进一步读取数据
	{
		return NO_REQUEST;
	}
	else
	{
		return BAD_REQUEST;
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

	struct sockaddr_in address;
	bzero(&address, sizeof(address));
	address.sin_family = AF_INET;
	inet_pton(AF_INET, ip, &address.sin_addr);
	address.sin_port = htons(port);

	int listenfd = socket(PF_INET, SOCK_STREAM, 0);
	assert(listenfd >= 0);
	int ret = bind(listenfd,(const sockaddr*)&address, sizeof(address));
	assert(ret != -1);
	ret = listen(listenfd, 5);
	assert(ret != -1);

	struct sockaddr_in client_address;
	socklen_t client_addrlength = sizeof(client_address);
	int fd = accept(listenfd, (sockaddr*)&client_address, &client_addrlength);
	if (fd < 0)
	{
		printf("errno is: %d\n", errno);
	}
	else
	{
		char buffer[BUFFER_SIZE];	// 读缓冲区
		memset(buffer, '\0', BUFFER_SIZE);
		int data_read = 0;			
		int read_index = 0;			// 当前已经读取了多少字节的客户数据
		int checked_index = 0;		// 当前已经分析了多少字节的客户数据
		int start_line = 0;			// 行在buffer中的起始位置

		CHECK_STATE checkstate = CHECK_STATE_REQUESTLINE;	// 主状态机的初始状态
		while (1)	// 循环读取数据并分析
		{
			data_read = recv(fd, buffer + read_index, BUFFER_SIZE - read_index, 0);
			if (data_read == -1)
			{
				printf("reading failed!\n");
				break;
			}
			else if (data_read == 0)
			{
				printf("remote client has closed the connection\n");
				break;
			}
			
			read_index += data_read;

			// 分析目前已经获得的所有客户数据
			HTTP_CODE result = parse_content(buffer, checked_index, checkstate, read_index, start_line);
			if (result == NO_REQUEST)
			{
				continue;
			}
			else if (result == GET_REQUEST)
			{
				send(fd, szret[0], strlen(szret[0]), 0);
				break;
			}
			else
			{
				send(fd, szret[1], strlen(szret[1]), 0);
				break;
			}
		}
		close(fd);
	}
	close(listenfd);
	return 0;
}