#ifndef __EVENTPOLL_H__
#define __EVENTPOLL_H__

#include <reworks/types.h>


/* Valid opcodes to issue to epoll_ctl() */

#define EPOLLIN 0x001
#define EPOLLPRI 0x002
#define EPOLLOUT 0x004
#define EPOLLRDNORM 0x040
#define EPOLLRDBAND 0x080
#define EPOLLWRNORM 0x100
#define EPOLLWRBAND 0x200
#define EPOLLMSG 0x400
#define EPOLLERR 0x008
#define EPOLLHUP 0x010
#define EPOLLRDHUP 0x2000
//#define EPOLLEXCLUSIVE (1U<<28)
//#define EPOLLWAKEUP (1U<<29)
//#define EPOLLONESHOT (1U<<30)
//#define EPOLLET (1U<<31)

#define EPOLL_EVT_MASK	0x001f

#define EPOLL_CTL_ADD 1
#define EPOLL_CTL_DEL 2
#define EPOLL_CTL_MOD 3


/*
 * Request the handling of system wakeup events so as to prevent system suspends
 * from happening while those events are being processed.
 *
 * Assuming neither EPOLLET nor EPOLLONESHOT is set, system suspends will not be
 * re-allowed until epoll_wait is called again after consuming the wakeup
 * event(s).
 *
 * Requires CAP_BLOCK_SUSPEND
 */
#define EPOLLWAKEUP (1 << 29) //lijuan: unsupport

/* Set the One Shot behaviour for the target file descriptor */
#define EPOLLONESHOT (1 << 30) //lijuan: unsupport

/* Set the Edge Triggered behaviour for the target file descriptor */
#define EPOLLET (1 << 31) //lijuan: unsupport

typedef union epoll_data {
	void *ptr;
	int fd;
	uint32_t data_u32;
	uint64_t data_u64;
} epoll_data_t;

struct epoll_event
{
	uint32_t events;
	epoll_data_t data;
};

/**
 * @brief  创建eventpoll对象
 * 
 * 该接口用于创建eventpoll对象，并返回对应的文件描述符。
 *
 * @param	max_fds		eventpoll对象支持的最大监控描述符数目
 *	
 * @return	eventpoll对象的文件描述符 	函数执行成功
 * @return 	-1	函数执行失败
 * 
 * @exception EMNOTINITED 	eventpoll模块尚未初始化
 * @exception ECALLEDINISR 	接口在中断上下文中调用
 * @exception EINVAL		max_fds的取值不合法
 * @exception ENOMEM		没有足够的内存
 * @exception ENFILE		系统中打开的文件描述符超过了最大值 
 */
extern int epoll_create(int size);

/**
 * @brief  添加/修改/删除需要监听的文件描述符及其事件
 * 
 * 该接口用于添加/修改/删除需要监听的文件描述符及其事件。
 *
 * @param	epfd		eventpoll对象的文件描述符
 * @param	op			执行的操作码，取值包括： <br/>
 * 						- EPOLL_CTL_ADD，添加要监控的文件描述符及事件 <br/>
 * 						- EPOLL_CTL_DEL，删除已添加的文件描述符及事件<br/>
 * 						- EPOLL_CTL_MOD，修改已添加的文件描述符及事件<br/>
 * @param	fd			要监控的文件描述符
 * @param 	event		要监控的文件描述符上的事件，取值包括：
 *						- EPOLLIN，表示fd对应的文件描述符可以读
 *						- EPOLLOUT，表示fd对应的文件描述符可以写
 *						- EPOLLPRI，表示fd对应的文件描述符有紧急数据可以读
 *						- EPOLLHUP，表示fd对应的文件描述符被挂断
 *						- EPOLLERR，表示fd对应的文件描述符发生错误

 * @return	0 	函数执行成功
 * @return 	-1	函数执行失败
 * 
 * @exception EMNOTINITED 	eventpoll模块尚未初始化
 * @exception ECALLEDINISR 	接口在中断上下文中调用
 * @exception EINVAL		参数取值不合法
 * @exception EBADF			参数文件描述符epfd或fd无效
 * @exception EEXIST		参数fd已存在于eventpoll对象，不可再次加入
 * @exception ENOENT		参数fd未存在于eventpoll对象，不可执行修改或删除
 */
extern int epoll_ctl(int epfd, int op, int fd,
				struct epoll_event *event);

/**
 * @brief  收集发生在被监听描述符上的、用户感兴趣的IO事件
 * 
 * 该接口用于收集发生在被监听描述符上的、用户感兴趣的IO事件。
 *
 * @param	epfd		eventpoll对象的文件描述符
 * @param	events		存放监控事件的数组
 * @param	maxevents	参数events指定的事件数组的大小
 * @param 	timeout		超时时间，单位为毫秒，取值为0时表示立即返回；取值为-1时表示永久阻塞

 * @return	0 	函数执行超时
 * @return 	-1	函数执行失败
 * @return 	>0	函数执行成功，返回收集到的事件数目
 * 
 * @exception EMNOTINITED 	eventpoll模块尚未初始化
 * @exception ECALLEDINISR 	接口在中断上下文中调用
 * @exception EINVAL		参数取值不合法
 */
extern int epoll_wait(int epfd, struct epoll_event  *events,
				int maxevents, int timeout);

#endif /*__EVENTPOLL_H__*/
