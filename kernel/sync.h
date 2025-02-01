#ifndef __THREAD_SYNC_H__
#define __THREAD_SYNC_H__

#include "stdint.h"
#include "kernel/list.h"
#include "thread.h"

struct semaphore {
    uint8_t value;
    struct list waiters;
};

// 锁结构
struct lock {
    struct task_struct *holder;
    struct semaphore semaphore;
    uint32_t holder_repeat_nr;
};

void lock_init(struct lock * plock);
void lock_acquire(struct lock * plock);
void lock_release(struct lock * plock);
void sema_init(struct semaphore * psema, uint8_t value);
void sema_up(struct semaphore * psema);
void sema_down(struct semaphore * psema);

#endif /* __THREAD_SYNC_H__ */