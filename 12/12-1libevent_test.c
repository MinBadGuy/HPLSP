#include <sys/signal.h>
#include <event.h>

#include <event2/event_struct.h>


void signal_cb(int fd, short event, void* argc)
{
    struct event_base* base = (struct event_base*)argc;
    struct timeval delay = {2, 0};
    printf("Caught an interrupt signal; exiting cleanly in two seconds...\n");
    event_base_loopexit(base, &delay);
}

void timeout_cb(int fd, short event, void* argc)
{
    printf("timeout\n");
}

int main()
{   
    /* 创建event_base对象，一个event_base相当于一个Reactor实例 */
    struct event_base* base = event_init();

    /* 创建信号事件处理器，每个事件处理器都需要关联到特定的event_base实例上去 */
    struct event* signal_event = evsignal_new(base, SIGINT, signal_cb, base);
    /* 将事件处理器添加到注册事件队列中，并将该事件处理器对应的事件添加到事件多路分发器 */
    event_add(signal_event, NULL);

    struct timeval tv = {1, 0};
    /* 创建定时事件处理器 */
    struct event* timeout_event = evtimer_new(base, timeout_cb, NULL);
    event_add(timeout_event, &tv);

    /* 执行事件循环 */
    event_base_dispatch(base);

    /* 释放系统资源 */
    event_free(timeout_event);
    event_free(signal_event);
    event_base_free(base);
}