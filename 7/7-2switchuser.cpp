#include <unistd.h>
#include <stdio.h>

static bool switch_to_user(uid_t user_id, gid_t gp_id)
{
    /* 先确保目标用户不是root */
    if ((user_id == 0) && (gp_id == 0))
    {
        return false;
    }

    /* 确保当前用户是合法用户：root或者目标用户 */
    gid_t gid = getgid();
    uid_t uid = getuid();
    printf("Before switch, user id: %d, group id: %d\n", uid, gid);
    if (((gid != 0) || (uid != 0)) && ((gid != gp_id) || (uid != user_id)))
    {
        return false;
    }

    /* 如果不是root，则已经是目标用户 */
    if (uid != 0)
    {
        return true;
    }

    /* 切换到目标用户 */
    if ((setgid(gp_id) < 0) || (setuid(user_id) < 0))
    {
        return false;
    }

    gid = getgid();
    uid = getuid();
    printf("After switch, user id: %d, group id: %d\n", uid, gid);
    
    return true;
}


int main()
{
    switch_to_user(1000, 1000);
}