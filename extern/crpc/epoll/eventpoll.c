#include <reworks/types.h>
#include <errno.h>
#include <pthread.h>
#include <driver.h>
#include <file.h>
#include <filep.h>
#include <ucore/thread-private.h>
#ifdef REWORKS_RTP
#include <ucore/rtp_private.h>
#endif
#include "reworksiop.h"
#include "eventpoll.h"

/*
 * This structure is stored inside the "private_data" member of the file
 * structure and represents the main data structure for the eventpoll
 * interface.
 */
struct eventpoll {
	/*
	 * This mtx is used to ensure that files are not removed
	 * while epoll is using them. This is held during the event
	 * collection loop, the file cleanup path, the epoll file exit
	 * code and the ctl operations.
	 */
	pthread_mutex_t mtx;

	fd_set		read_fds;
	fd_set		write_fds;
	fd_set		err_fds;
	
	fd_set		watch_fds;
	
	int 		max_fds;
	int 		curr_fds;
	int 		max_fdno;
};

/**
 * 文件操作
 */
static int epoll_free(struct eventpoll *epoll_ptr);

struct file_operations epoll_file_ops =
{
	close	: (file_close)epoll_free
};

static int epoll_inited = 0;

#define CHECK_MUTEX_MODULE_INIT(retn)	\
	do {	\
		if (UNLIKELY(!epoll_inited)) \
		{	\
			errno = EMNOTINITED; \
			return retn;	\
		}\
	}while(0);

/**
 * @brief  eventpoll模块的初始化
 * 
 * 该接口用于初始化eventpoll模块，一般由系统在初始化时调用。
 *
 * @param	无
 *	
 * @return	0 	函数执行成功
 * @return 	-1	函数执行失败
 * 
 * @exception EMINITED eventpoll模块已被初始化
 *
 */
int eventpoll_module_init()
{
	/*  */	
	if (epoll_inited)
	{
		errno = EMINITED;
		
		return -1;
	}
	
	epoll_inited = 1;
	return 0;
}

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
int epoll_create(int max_fds)
{
	int				fd;			/* 待打开文件的文件描述符 */
	struct file     *filp = 0;	/* 文件描述结构指针 */
	struct eventpoll *epoll_ptr;
	int 			ret;
	
	/* 1. 判断模块是否初始化 */
	CHECK_MUTEX_MODULE_INIT(-1);
	
	/* 2. 判断是否为中断上下文 */
	if (isr_nest_level() > 0)
	{
		errno = ECALLEDINISR;
		return -1;
	}
	
	/* 3. 判断参数有效性 */
	if (max_fds <= 0 || max_fds > FD_SETSIZE)
	{
		errno = EINVAL;
		return -1;
	}
	
	/* 4. 申请struct file */
	if (NULL == (filp = file_alloc()))
	{
		return -1;
	}
    
	/* 5. 创建eventpoll对象，并初始化 */
	epoll_ptr = (struct eventpoll*)kmalloc(sizeof(struct eventpoll));
	if (epoll_ptr == NULL)
    {
    	file_free(filp);
    	
    	/* 错误号由底层设定 */
		return -1;	    	
    }
	
	memset(epoll_ptr, 0, sizeof(struct eventpoll));
	epoll_ptr->mtx = PTHREAD_MUTEX_INITIALIZER;
	ret = pthread_mutex_init(&epoll_ptr->mtx, NULL);
	if (ret != 0)
	{
		kfree(epoll_ptr);
    	file_free(filp);
    	
    	/* 错误号由底层设定 */
    	return -1;
	}
	
	FD_ZERO(&(epoll_ptr->read_fds));
	FD_ZERO(&(epoll_ptr->write_fds));
	FD_ZERO(&(epoll_ptr->err_fds));

	FD_ZERO(&(epoll_ptr->watch_fds));
	
	epoll_ptr->max_fds = max_fds;
	
	filp->private_data = epoll_ptr;
	filp->f_ops = &epoll_file_ops;
	
    /* 6. 获取文件描述符 */
    if (-1 == (fd = set_file(filp)))
    {
    	printf("[ERROR] too many files opened\n");
    	pthread_mutex_destroy(&epoll_ptr->mtx);
		kfree(epoll_ptr);

    	file_free(filp);    	
    	set_errno_retn(ENFILE, -1);
    }
    else
    {
#ifdef REWORKS_RTP
    	FD_SET(fd, &((RTP_CTRL*)(((NThread_Ctrl*)THREAD_CURRENT_GET)->rtp_id))->fds_bit);
#else
    	FD_SET(fd, &open_fds);
#endif
    }
    
    filp->f_fd = fd;

    return fd;
}

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
int epoll_ctl(int epfd, int op, int fd,
				struct epoll_event *event)
{
	struct file* 	fp = NULL, *tfp = NULL;
	struct eventpoll *ep = NULL;
	
	/* 1. 判断模块是否初始化 */
	CHECK_MUTEX_MODULE_INIT(-1);
	
	/* 2. 判断是否为中断上下文 */
	if (isr_nest_level() > 0)
	{
		errno = ECALLEDINISR;
		return -1;
	}
	
	/* 3. 判断参数有效性 */
	if ((op != EPOLL_CTL_ADD &&
		op != EPOLL_CTL_DEL &&
		op != EPOLL_CTL_MOD) ||
		event == NULL ||
		!is_va_valid(event) ||
		(event->events & ~(EPOLL_EVT_MASK)) != 0)
	{ /* 参数op取值无效 或 参数event取值无效 */
		errno = EINVAL;
		return -1;
	}
	
	tfp = get_file(fd);
	if (tfp == NULL ||
		tfp->f_ops == &epoll_file_ops)
	{ /* 参数fd取值无效 或 参数fd为epoll文件描述符 */
		errno = EBADF;
		return -1;
	}
	
	fp = get_file(epfd);
	if (fp == NULL)
	{ /* 参数epfp取值无效 */
		errno = EBADF;
		return -1;
	}
	if (fd == epfd || fp->f_ops != &epoll_file_ops)
	{ /* 参数epfp不为epoll文件描述符 */
		errno = EINVAL;
		return -1;
	}

	ep = (struct eventpoll *)fp->private_data;
	pthread_mutex_lock(&(ep->mtx));
	if (ep->max_fds == -1 || ep->curr_fds >= ep->max_fds)
	{/* 参数fp取值大于eventpoll对象的最大监控fd值 */
		pthread_mutex_unlock(&(ep->mtx));
		errno = EINVAL;
		return -1;
	}
	
	if (FD_ISSET(fd, &(ep->watch_fds)))
	{/* 参数fd已存在于eventpoll对象中 */
		if (op == EPOLL_CTL_ADD)
		{
			pthread_mutex_unlock(&(ep->mtx));
			errno = EEXIST;
			return -1;
		}
	}
	else 
	{
		if (op == EPOLL_CTL_DEL || op == EPOLL_CTL_MOD)
		{
			pthread_mutex_unlock(&(ep->mtx));
			errno = ENOENT;
			return -1;
		}
	}
	
	/* 4. 根据op处理fd&event*/
	switch(op)
	{
	case EPOLL_CTL_ADD:
	{
		if (event->events & (EPOLLIN | EPOLLPRI | EPOLLRDNORM | EPOLLRDBAND))
		{
			FD_SET(fd, &(ep->read_fds));
		}
		
		if (event->events & (EPOLLOUT | EPOLLWRNORM | EPOLLWRBAND))
		{
			FD_SET(fd, &(ep->write_fds));
		}
		
		if (event->events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
		{
			FD_SET(fd, &(ep->err_fds));
		}
		
		FD_SET(fd, &(ep->watch_fds));
		
		ep->curr_fds++;
		if (fd > ep->max_fdno)
		{
			ep->max_fdno = fd;
		}
		
		break;
	}
	case EPOLL_CTL_DEL:
	{
		if (event->events & (EPOLLIN | EPOLLPRI | EPOLLRDNORM | EPOLLRDBAND))
		{
			FD_CLR(fd, &(ep->read_fds));
		}
		
		if (event->events & (EPOLLOUT | EPOLLWRNORM | EPOLLWRBAND))
		{
			FD_CLR(fd, &(ep->write_fds));
		}
		
		if (event->events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
		{
			FD_CLR(fd, &(ep->err_fds));
		}
		
		FD_CLR(fd, &(ep->watch_fds));
		
		ep->curr_fds--;
		if (ep->curr_fds == 0)
		{
			ep->max_fdno = 0;
		}
		else if (fd == ep->max_fdno)
		{
			ep->max_fdno--;
		}
		
		break;
	}
	case EPOLL_CTL_MOD:
	{
		if (event->events & (EPOLLIN | EPOLLPRI | EPOLLRDNORM | EPOLLRDBAND))
		{
			FD_SET(fd, &(ep->read_fds));
		}
		else
		{
			FD_CLR(fd, &(ep->read_fds));
		}
		
		if (event->events & (EPOLLOUT | EPOLLWRNORM | EPOLLWRBAND))
		{
			FD_SET(fd, &(ep->write_fds));
		}
		else
		{
			FD_CLR(fd, &(ep->write_fds));
		}

		if (event->events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
		{
			FD_SET(fd, &(ep->err_fds));
		}
		else
		{
			FD_CLR(fd, &(ep->err_fds));
		}

		break;
	}
	}
	
	pthread_mutex_unlock(&(ep->mtx));

	return 0;
}

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
int epoll_wait(int epfd, struct epoll_event  *events,
				int maxevents, int timeout)
{
	struct file* 	fp = NULL;
	struct eventpoll *ep = NULL;
	int 			ret = -1;
	int				fdset_size;
	int				max_fdno = 0, fd_idx = 0;
	int				event_idx = 0;
	fd_set			rdfds, wrfds, errfds; 
	struct timeval	timeout_val;
	
	/* 1. 判断模块是否初始化 */
	CHECK_MUTEX_MODULE_INIT(-1);
	
	/* 2. 判断是否为中断上下文 */
	if (isr_nest_level() > 0)
	{
		errno = ECALLEDINISR;
		return -1;
	}
	
	/* 3. 判断参数有效性 */
	if (events == NULL ||
		!is_va_valid(events) ||
		maxevents <= 0 ||
		timeout < -1)
	{ /*  参数events或maxevents或timeout取值无效 */
		errno = EINVAL;
		return -1;
	}
	
	fp = get_file(epfd);
	if (fp == NULL)
	{ /* 参数epfp取值无效 */
		errno = EBADF;
		return -1;
	}
	if (fp->f_ops != &epoll_file_ops)
	{ /* 参数epfp不为epoll文件描述符 */
		errno = EINVAL;
		return -1;
	}
	
	ep = (struct eventpoll *)fp->private_data;
	
	pthread_mutex_lock(&(ep->mtx));
	
	max_fdno = ep->max_fdno;
	if (max_fdno == -1)
	{
		pthread_mutex_unlock(&(ep->mtx));
		errno = EINVAL;
		return -1;
	}
	
	fdset_size = FDS_BYTES(max_fdno);
	bcopy(&ep->read_fds, &rdfds, fdset_size);
	bcopy(&ep->write_fds, &wrfds, fdset_size);
	bcopy(&ep->err_fds, &errfds, fdset_size);
	
	pthread_mutex_unlock(&(ep->mtx));

	if (timeout == -1)
	{
		ret = select(max_fdno+1, &rdfds, &wrfds, &errfds, NULL);
	}
	else
	{
		timeout_val.tv_sec = timeout / 1000;
		timeout_val.tv_usec = (timeout % 1000) * 1000;
		ret = select(max_fdno+1, &rdfds, &wrfds, &errfds, &timeout_val);
	}
	
	if (ret == -1)
	{
		if (errno == ETIMEDOUT)
		{
			return 0;
		}
		return -1;
	}
	
	for (fd_idx = 0; fd_idx < max_fdno+1; fd_idx++)
	{
		if (FD_ISSET(fd_idx, &rdfds))
		{
			events[event_idx].data.fd = fd_idx;
			events[event_idx++].events = EPOLLIN;
		}
		if (FD_ISSET(fd_idx, &wrfds))
		{
			events[event_idx].data.fd = fd_idx;
			events[event_idx++].events = EPOLLOUT;
		}
		if (FD_ISSET(fd_idx, &errfds))
		{
			events[event_idx].data.fd = fd_idx;
			events[event_idx++].events = EPOLLERR;
		}
		
		if (event_idx >= maxevents || event_idx >= ret)
		{
			break;
		}
	}
	
	return event_idx;
}


static int epoll_free(struct eventpoll *epoll_ptr)
{
	pthread_mutex_lock(&epoll_ptr->mtx);
	
	FD_ZERO(&(epoll_ptr->read_fds));
	FD_ZERO(&(epoll_ptr->write_fds));
	FD_ZERO(&(epoll_ptr->err_fds));

	FD_ZERO(&(epoll_ptr->watch_fds));
	
	epoll_ptr->max_fds = -1;
	pthread_mutex_unlock(&epoll_ptr->mtx);

	pthread_mutex_destroy(&epoll_ptr->mtx);
	
	kfree(epoll_ptr);
	
	return 0;
}
