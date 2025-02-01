#include "all.h"

void sema_init(struct semaphore * psema, uint8_t value){
    psema->value = value;  // 初始化信号量的值
    list_init(&psema->waiters);  // 初始化等待队列
}

void lock_init(struct lock * plock){
    plock->holder = NULL;
    plock->holder_repeat_nr = 0;
    sema_init(&plock->semaphore, 1); // 信号量初始化为1，表示可用
}

void sema_down(struct semaphore * psema){
    enum intr_status old_status = intr_disable();  // 关闭中断
    while(psema->value == 0)
    {
        ASSERT(!list_find(&psema->waiters, &running_thread()->general_tag));
        if(list_find(&psema->waiters, &running_thread()->general_tag)){
            PANIC("[sema_down] thread blocked has been waiters_list\n");
        }
        list_append(&psema->waiters, &running_thread()->general_tag);
        thread_block(TASK_BLOCKED);  // 阻塞当前线程
    }
    psema->value--;
    ASSERT(psema->value == 0);
    intr_set_status(old_status);  // 恢复中断
}

void sema_up(struct semaphore * psema){
    enum intr_status old_status = intr_disable();  // 关闭中断
    ASSERT(psema->value == 0);
    if (!list_empty(&psema->waiters)){
        struct task_struct * thread_blocked = elem2entry(struct task_struct, general_tag, list_pop(&psema->waiters));
        thread_unblock(thread_blocked);
    }
    psema->value++;
    ASSERT(psema->value == 1);
    intr_set_status(old_status);  // 恢复中断
}

void lock_acquire(struct lock * plock){
    if(plock->holder != running_thread()){
        sema_down(&plock->semaphore);
        plock->holder = running_thread();
        ASSERT(plock->holder_repeat_nr == 0);
        plock->holder_repeat_nr = 1;
    }
    else{
        plock->holder_repeat_nr++;
    }
}

void lock_release(struct lock * plock){
    ASSERT(plock->holder == running_thread());
    if(plock->holder_repeat_nr > 1){
        plock->holder_repeat_nr--;
        return;
    }
    ASSERT(plock->holder_repeat_nr == 1);

    plock->holder = NULL;
    plock->holder_repeat_nr = 0;
    sema_up(&plock->semaphore); // 信号量的V操作，也是原子操作
}