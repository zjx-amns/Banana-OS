# include "thread.h"
# include "stdint.h"
# include "string.h"
# include "global.h"
# include "memory.h"
# include "debug.h"
# include "interrupt.h"
# include "kernel/list.h"
# include "kernel/print.h"
#include "log.h"

# define PAGE_SIZE 4096

struct task_struct* main_thread; // 主线程PCB
struct list thread_ready_list;   // 就绪队列
struct list thread_all_list;     // 所有线程队列
static struct list_elem* thread_tag; // 用于保存队列中的线程结点

extern void switch_to(struct task_struct* cur, struct task_struct* next);

struct task_struct* running_thread() {
    uint32_t esp;
    asm ("mov %%esp, %0" : "=g" (esp));
    return (struct task_struct*) (esp & 0xfffff000);
}

static void kernel_thread(thread_func* function, void* func_args){
    intr_enable();
    function(func_args);
}

/**
 * 初始化线程栈.
 */ 
void thread_create(struct task_struct* pthread, thread_func function, void* func_args) {
    pthread->self_kstack -= sizeof(struct intr_stack);
    pthread->self_kstack -= sizeof(struct thread_stack);

    struct thread_stack* kthread_stack = (struct thread_stack*) pthread->self_kstack;
    kthread_stack->eip = kernel_thread;
    kthread_stack->function = function;
    kthread_stack->func_args = func_args;
    kthread_stack->ebp = kthread_stack->ebx = kthread_stack->edi = kthread_stack->esi = 0;
}

/**
 * 初始化线程基本信息.
 */ 
void init_thread(struct task_struct* pthread, char* name, int prio) {
    memset(pthread, 0, sizeof(*pthread));
    strcpy(pthread->name, name);
    
    if (pthread == main_thread) {
        pthread->status = TASK_RUNNING;
    } else {
        pthread->status = TASK_READY;
    }
    
    pthread->status = TASK_RUNNING;
    pthread->priority = prio;
    pthread->ticks = prio;
    pthread->elaspsed_ticks = 0;
    pthread->pgdir = NULL;
    // PCB所在物理页的顶端地址
    pthread->self_kstack = (uint32_t*) ((uint32_t) pthread + PAGE_SIZE);
    pthread->stack_magic = 0x19870916;
}

/**
 * 创建线程， 优先级为prio， 名称为name.
 */ 
struct task_struct* thread_start(char* name, int prio, thread_func function, void* func_args) {
    struct task_struct* thread = get_kernel_pages(1);

    init_thread(thread, name, prio);
    thread_create(thread, function, func_args);

    ASSERT(!list_find(&thread_ready_list, &thread->general_tag));
    list_append(&thread_ready_list, &thread->general_tag);

    ASSERT(!list_find(&thread_all_list, &thread->all_list_tag));
    list_append(&thread_all_list, &thread->all_list_tag);
    return thread;
}

// 将kernel中的main函数完善为主线程
static void make_main_thread(void){
    main_thread = running_thread();
    init_thread(main_thread, "main", 31);
    ASSERT(!list_find(&thread_all_list, &main_thread->all_list_tag));
    list_append(&thread_all_list, &main_thread->all_list_tag);
}

void schedule() {
    ASSERT(intr_get_status() == INTR_OFF);

    struct task_struct* cur_thread = running_thread();
    if (cur_thread->status == TASK_RUNNING) {
        // 时间片用完，重新加到就绪队列
        ASSERT(!list_find(&thread_ready_list, &cur_thread->general_tag));
        list_append(&thread_ready_list, &cur_thread->general_tag);
        cur_thread->ticks = cur_thread->priority;
        cur_thread->status = TASK_READY;
    }
    
    // 当前没有实现idle线程，所以要保证必须有可调度的线程存在
    ASSERT(!list_empty(&thread_ready_list));

    thread_tag = NULL;
    thread_tag = list_pop(&thread_ready_list);
    struct task_struct* next = elem2entry(struct task_struct, general_tag, thread_tag);
    next->status = TASK_RUNNING;
    
    switch_to(cur_thread, next);
}

void thread_block(enum task_status stat){
    ASSERT(((stat == TASK_BLOCKED)||(stat == TASK_WAITING)||(stat == TASK_HANGING)));

    enum intr_status old_status = intr_disable();
    struct task_struct* cur_thread = running_thread();
    cur_thread->status = stat;
    schedule();
    intr_set_status(old_status);
}

void thread_unblock(struct task_struct* pthread){
    enum intr_status old_status = intr_disable();  // 关闭中断
    ASSERT(pthread->status == TASK_BLOCKED || pthread->status == TASK_WAITING || pthread->status == TASK_HANGING);
    if (pthread->status != TASK_READY){
        if (list_find(&thread_ready_list, &pthread->general_tag)){
            PANIC("thread_unblock: thread %s already in ready list\n");
        }
        list_push(&thread_ready_list, &pthread->general_tag);
        pthread->status = TASK_READY;
    }
    intr_set_status(old_status);  // 开启中断
}
/**
 * 线程模块初始化.
 */ 
void thread_init() {
    mylog("Thread init start.\n", 5);
    list_init(&thread_all_list);
    list_init(&thread_ready_list);
    make_main_thread();
    mylog("Thread init end.\n", 1);
}