#include<stdio.h>

/* 判断机器字节序 */
void byteorder()
{
	union
	{
		short value;
		char union_bytes[sizeof(short)];
	} test;
	
	test.value = 0x0102;

	if ((test.union_bytes[0] == 1) &&(test.union_bytes[1] == 2))		// 大端字节序：低地址存放高位字节
	{
		printf("big endian\n");
	}
	else if ((test.union_bytes[0] == 2) && (test.union_bytes[1] == 1))	// 小端字节序：低地址存放低位字节
	{
		printf("little endian\n");
	}
	else
	{
		printf("unknow...\n");
	}
}

int main()
{
	byteorder();
}