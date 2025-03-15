#include "kernel/pic.h"
#include "kernel/x86.h"
#include "kernel/kernel.h"
#include "kernel/scheduler.h"

#define PIT_CTRL 0x43
#define PIT_CH0 0x40
#define OSC_FREQ 1193180 // PIT晶振频率
#define HZ 1             // 时钟中断频率

void start_timer(void)
{
    // 设置模式3（方波发生器），二进制计数
    outb(PIT_CTRL, 0x36);

    // 设置分频值
    uint16_t divisor = OSC_FREQ / HZ;
    outb(PIT_CH0, divisor & 0xFF);
    outb(PIT_CH0, (divisor >> 8) & 0xFF);

    // 启用时钟中断
    pic_enable_irq(0);
}

void timer_handler(interrupt_frame *frame)
{
    DEBUGK("timer interrupt");

    // 执行调度切换任务
    schedule(frame);
}

void handled_timer(void)
{
    // 发送中断处理结束信号
    pic_send_eoi(0);
}