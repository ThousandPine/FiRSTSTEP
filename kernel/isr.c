#include "kernel/kernel.h"

/**
 * CPU 中断服务程序
 * 对应 CPU 保留的 0-31 号中断服务
 */

// Divide Error
__attribute__((naked)) void isr_de(void)
{
    panic("#DE");
}
// Debug
__attribute__((naked)) void isr_db(void)
{
    panic("#DB");
}
// Non-Maskable Interrupt
__attribute__((naked)) void isr_nmi(void)
{
    panic("NMI");
}
// Breakpoint
__attribute__((naked)) void isr_bp(void)
{
    panic("#BP");
}
// Overflow
__attribute__((naked)) void isr_of(void)
{
    panic("#OF");
}
// Bound Range Exceeded
__attribute__((naked)) void isr_br(void)
{
    panic("#BR");
}
// Invalid Opcode
__attribute__((naked)) void isr_ud(void)
{
    panic("#UD");
}
// Device Not Available
__attribute__((naked)) void isr_nm(void)
{
    panic("#NM");
}
// Double Fault
__attribute__((naked)) void isr_df(void)
{
    panic("#DF");
}
// Coprocessor Segment Overrun (reserved, not used)
__attribute__((naked)) void isr_cso(void)
{
    panic("Coprocessor Segment Overrun");
}
// Invalid TSS
__attribute__((naked)) void isr_ts(void)
{
    panic("#TS");
}
// Segment Not Present
__attribute__((naked)) void isr_np(void)
{
    panic("#NP");
}
// Stack-Segment Fault
__attribute__((naked)) void isr_ss(void)
{
    panic("#SS");
}
// General Protection Fault
__attribute__((naked)) void isr_gp(void)
{
    panic("#GP");
}
// Page Fault
__attribute__((naked)) void isr_pf(void)
{
    panic("#PF");
}
// x87 Floating-Point Exception
__attribute__((naked)) void isr_mf(void)
{
    panic("#MF");
}
// Alignment Check
__attribute__((naked)) void isr_ac(void)
{
    panic("#AC");
}
// Machine Check
__attribute__((naked)) void isr_mc(void)
{
    panic("#MC");
}
// SIMD Floating-Point Exception
__attribute__((naked)) void isr_xm(void)
{
    panic("#XM");
}
// Virtualization Exception
__attribute__((naked)) void isr_ve(void)
{
    panic("#VE");
}
// Control Protection Exception
__attribute__((naked)) void isr_cp(void)
{
    panic("#CP");
}
// Reserved Exception
__attribute__((naked)) void isr_reserved(void)
{
    panic("Reserved Exception");
}

/**
 * 自定义中断服务程序
 * 对应可使用的 32-255 号中断服务
 */

// 默认中断服务程序
__attribute__((naked)) void isr_default(void)
{
    panic("Undefined interrupt handling");
}