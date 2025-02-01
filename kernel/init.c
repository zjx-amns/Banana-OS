#include "init.h"
#include "kernel/print.h"
#include "interrupt.h"
#include "timer.h"
#include "thread.h"
#include "log.h"
#include "memory.h"
#include "keyboard.h"
#include "console.h"

void init_all(){
	mylog("Start to init all\n",5);
	idt_init();  //初始化中断
	mem_init();   //初始化内存
	thread_init(); //初始化线程
	timer_init(); //初始化定时器
	console_init(); //初始化控制台
	keyboard_init(); //初始化键盘
	mylog("Init all done\n",1);
}