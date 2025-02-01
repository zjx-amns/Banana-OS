#include "interrupt.h"
#include "stdint.h"
#include "global.h"
#include "kernel/print.h"
#include "io.h"
#include "log.h"

# define IDT_DESC_CNT 0x30
# define PIC_M_CTRL 0x20
# define PIC_M_DATA 0x21
# define PIC_S_CTRL 0xa0
# define PIC_S_DATA 0xa1
# define EFLAGS_IF 0x00000200
# define GET_EFLAGS(EFLAG_VAR) asm volatile ("pushfl; popl %0" : "=g" (EFLAG_VAR))

struct gate_desc {
    uint16_t    func_offset_low_word;
    uint16_t    selector;
    uint8_t     dcount;

    uint8_t     attribute;
    uint16_t    func_offset_high_word;
};

//中断的名称
char* intr_name[IDT_DESC_CNT];
intr_handler idt_table[IDT_DESC_CNT];
static void make_idt_desc(struct gate_desc* p_gdesc, uint8_t attr, intr_handler function);
static struct gate_desc idt[IDT_DESC_CNT];

extern intr_handler intr_entry_table[IDT_DESC_CNT];

// 一般中断处理函数
static void general_intr_handler(uint8_t vec_nr) {
    if (vec_nr == 0x27 || vec_nr == 0x2f) {
        // 伪中断，无需处理
        return;
    }

    // 将光标置为0，清屏， 从屏幕左上角清出一片打印异常信息的区域，方便阅读
    set_cursor(0);
    int cursor_pos = 0;
    while (cursor_pos < 320){
        put_char(' ');
        cursor_pos++;
    }

    set_cursor(0);
    put_str("------------ excetion message begin ------------\n");
    set_cursor(88);  // 从第2行第8个字符开始打印
    put_str(intr_name[vec_nr]);
    if (vec_nr == 14){
        int page_fault_vaddr = 0;
        asm ("movl %%cr2, %0" : "=r" (page_fault_vaddr));

        put_str("\npage fault addr is ");
        put_int(page_fault_vaddr);
    }
    put_str("\n------------ excetion message end ------------\n");

    // 不会再出现调度进程的情况，直接进入死循环
    while (1);
    
}

/**
 * 开中断并返回之前的状态.
 */ 
enum intr_status intr_enable() {
    enum intr_status old_status;
    if (INTR_ON == intr_get_status()) {
        old_status = INTR_ON;
	return old_status;
    }

    old_status = INTR_OFF;
    asm volatile ("sti");
    return old_status;
}

/**
 * 关中断并返回之前的状态.
 */
enum intr_status intr_disable() {
    enum intr_status old_status;
    if (INTR_OFF == intr_get_status()) {
        old_status = INTR_OFF;
        return old_status;
    }

    old_status = INTR_ON;
    asm volatile ("cli" : : : "memory");
    return old_status;
}

/**
 * 获取中断状态.
 */ 
enum intr_status intr_get_status() {
    uint32_t eflags = 0;
    GET_EFLAGS(eflags);
    return (EFLAGS_IF & eflags) ? INTR_ON : INTR_OFF;
}

enum intr_status intr_set_status(enum intr_status status) {
    return status & INTR_ON ? intr_enable() : intr_disable();
}

// 完成一般中断处理函数注册及异常名称注册
static void exception_init(void) {
    int i;
    for (i = 0; i < IDT_DESC_CNT; i++)
    {
        idt_table[i] = general_intr_handler;
        intr_name[i] = "unknown";  // 先全部初始化为unknown
    }
    intr_name[0] = "Divide Error";                      // 除法错误
    intr_name[1] = "Debug Exception";                   // 调试异常
    intr_name[2] = "NMI Interrupt";                     // 非屏蔽中断
    intr_name[3] = "Breakpoint Exception";              // 断点异常
    intr_name[4] = "Overflow Exception";                // 溢出异常
    intr_name[5] = "BOUND Range Exceeded Exception";    // BOUND范围超出异常
    intr_name[6] = "Invalid Opcode Exception";          // 无效的指令异常
    intr_name[7] = "Device Not Available Exception";    // 设备不可用异常
    intr_name[8] = "Double Fault Exception";            // 双重故障异常
    intr_name[9] = "Coprocessor Segment Overrun";       // 协处理器段越界
    intr_name[10] = "Invalid TSS Exception";            // 无效的TSS异常
    intr_name[11] = "Segment Not Present";              // 段不存在
    intr_name[12] = "Stack Fault Exception";            // 栈故障异常
    intr_name[13] = "General Protection Exception";     // 通用保护异常
    intr_name[14] = "Page-Fault Exception";             // 页错误异常
                                                        // 第15号中断是Intel保留的，未使用
    intr_name[16] = "0x87 FPU Floating-Point Error";    // 0x87 FPU浮点错误
    intr_name[17] = "Alignment Check Exception";        // 对齐检查异常
    intr_name[18] = "Machine-Check Exception";          // 机器检查异常
    intr_name[19] = "SIMD Floating-Point Exception";    // SIMD浮点异常
    mylog("        exception_init done.\n", 1);
}

static void make_idt_desc(struct gate_desc* p_gdesc, uint8_t attr, intr_handler function) {
    p_gdesc->func_offset_low_word = (uint32_t) function & 0x0000FFFF;
    p_gdesc->selector = SELECTOR_K_CODE;
    p_gdesc->dcount = 0;
    p_gdesc->attribute = attr;
    p_gdesc->func_offset_high_word = ((uint32_t) function & 0xFFFF0000) >> 16;
}

//初始化中断描述符表
static void idt_desc_init(void) {
    int i;
    for (i = 0; i < IDT_DESC_CNT; i++) {
        make_idt_desc(&idt[i], IDT_DESC_ATTR_DPL0, intr_entry_table[i]);
    }
    mylog("        idt_desc_init done.\n", 1);
}


static void pic_init(void) {
    // 初始化主片
    outb(PIC_M_CTRL, 0x11);
    outb(PIC_M_DATA, 0x20);

    outb(PIC_M_DATA, 0x04);
    outb(PIC_M_DATA, 0x01);

    outb(PIC_S_CTRL, 0x11);
    outb(PIC_S_DATA, 0x28);

    outb(PIC_S_DATA, 0x02);
    outb(PIC_S_DATA, 0x01);

    outb(PIC_M_DATA, 0xfd);
    outb(PIC_S_DATA, 0xff);

    mylog("        pic_init done.\n", 1);
}

// 注册中断处理函数 function
void register_handler(uint8_t vector_no, intr_handler function) {
    idt_table[vector_no] = function;
}

void idt_init() {
    mylog("    idt_init start.\n", 5);
    idt_desc_init();
    exception_init();
    pic_init();

    // 加载idt
    uint64_t idt_operand = ((sizeof(idt) - 1) | ((uint64_t) ((uint32_t) idt << 16)));
    asm volatile ("lidt %0" : : "m" (idt_operand));
    mylog("    idt_init done.\n", 1);
}