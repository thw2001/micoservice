#ifndef XSHM_LOCK_H
#define XSHM_LOCK_H

// 初始化锁
#if defined(__USE_SPINLOCK__)
#include "core/spinlock.h"
typedef struct spinlock shm_lock_t;
#define shm_lock_init(lock) spin_lock_init(lock)
#define shm_lock_acquire(lock) spin_lock(lock)
#define shm_lock_release(lock) spin_unlock(lock)
#else
typedef struct {
    volatile unsigned int lock;
} shm_lock_t;
#define shm_lock_init(lock)   0
#define shm_lock_acquire(lock)  0
#define shm_lock_release(lock)  0
// 尝试获取锁
#define shm_lock_try_acquire(lock)  0
#endif



#endif // SHM_LOCK_H 
