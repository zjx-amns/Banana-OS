# include "kernel/print.h"
# include "init.h"
# include "thread.h"
# include "interrupt.h"
#include "console.h"

void k_thread_function_a(void*);
void k_thread_function_b(void*);

int main(void) {
    print("[ Banana operating system ]\n");
    print("edition - v0.0.5\n");
    init_all();

    console_put_str("Test keyboard input:\n");

    //thread_start("k_thread_a", 31, k_thread_function_a, "threadA ");
    //thread_start("k_thread_b", 8, k_thread_function_b, "threadB ");

    intr_enable();

    while (1);

    return 0;
}

// void k_thread_function_a(void* args) {
//     // 这里必须是死循环，否则执行流并不会返回到main函数，所以CPU将会放飞自我，出发6号未知操作码异常
//     while (1) {
//         intr_disable();
//         put_str((char*) args);
//         intr_enable();
//     }
// }

// void k_thread_function_b(void* args) {
//     while (1) {
//         intr_disable();
//         put_str((char*) args);
//         intr_enable(); 
//     }
// }