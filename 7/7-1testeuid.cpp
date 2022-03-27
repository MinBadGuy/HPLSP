#include <unistd.h>
#include <stdio.h>

int main()
{
    uid_t uid = getuid();
    uid_t euid = geteuid();
    /* 进程的真实用户ID(UID)是启动程序的用户的ID，而有效用户ID(EUID)是文件所有者的ID */
    printf("userid is %d, effective userid is: %d\n", uid, euid);
    return 0;
}