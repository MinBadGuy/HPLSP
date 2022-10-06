#include <unistd.h>
#include <stdio.h>

int main()
{
    uid_t uid = getuid();   // 获取真实用户id
    uid_t euid = geteuid(); // 获取有效用户id
    /* 进程的真实用户ID(UID)是启动程序的用户的ID，而有效用户ID(EUID)是文件所有者的ID */
    printf("userid is %d, effective userid is: %d\n", uid, euid);
    return 0;
}