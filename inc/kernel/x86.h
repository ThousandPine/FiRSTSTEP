#pragma once

#include "types.h"

#define CR0_PG (1 << 31) // CR0 寄存器启用分页功能标志位

__attribute__((always_inline))
static inline uint8_t inb(uint16_t port)
{
    uint8_t value;
    asm volatile("inb %1, %0"
                 : "=a"(value)  // 输出到 AL 寄存器（存储结果）
                 : "Nd"(port)); // 输入端口号（立即数或 DX）
    return value;
}

__attribute__((always_inline))
static inline void insl(uint16_t port, void *addr, uint32_t count)
{
    asm volatile(
        "rep insl"    // 重复执行 `insl` 指令
        : "=D"(addr), // 输出操作数，更新 `EDI` 指针
          "=c"(count) // 输出操作数，`ECX` 剩余的次数
        : "d"(port),  // 输入操作数，端口地址存入 `DX`
          "0"(addr),  // 输入操作数，初始内存地址
          "1"(count)  // 输入操作数，数据个数
        : "memory"    // 告诉编译器，内存被修改
    );
}

__attribute__((always_inline))
static inline void outb(uint16_t port, uint8_t value)
{
    asm volatile("outb %0, %1"
                 :             // 没有输出操作数
                 : "a"(value), // 输入值，放入 AL
                   "Nd"(port)  // 输入端口号
    );
}

__attribute__((always_inline))
static inline void outsl(uint16_t port, const void *addr, uint32_t count)
{
    asm volatile(
        "rep outsl"   // 重复执行 `outsl` 指令
        : "=S"(addr), // 输出操作数，更新 `ESI` 指针
          "=c"(count) // 输出操作数，`ECX` 剩余的次数
        : "d"(port),  // 输入操作数，端口地址存入 `DX`
          "0"(addr),  // 输入操作数，初始内存地址
          "1"(count)  // 输入操作数，数据个数
        : "memory"    // 告诉编译器，内存被读取
    );
}

__attribute__((always_inline))
static inline uint32_t get_cr0(void)
{
    uint32_t value;
    asm volatile("mov %%cr0, %0" : "=r"(value));
    return value;
}

__attribute__((always_inline))
static inline void set_cr0(uint32_t value)
{
    asm volatile("mov %0,%%cr0" : : "r"(value));
}

__attribute__((always_inline))
static inline uint32_t get_cr3(void)
{
    uint32_t value;
    asm volatile("mov %%cr3, %0" : "=r"(value));
    return value;
}

__attribute__((always_inline))
static inline void set_cr3(uint32_t value)
{
    asm volatile("mov %0,%%cr3" : : "r"(value));
}

__attribute__((always_inline))
static inline void sti(void)
{
    asm volatile("sti" ::);
}

__attribute__((always_inline))
static inline void cli(void)
{
    asm volatile("cli" ::);
}

__attribute__((always_inline))
static inline void ltr(uint32_t value)
{
    asm volatile("ltr %0" : : "r"(value) : "memory");
}

__attribute__((always_inline))
static inline uint32_t get_eflags(void)
{
    uint32_t eflags;
    asm volatile (
        "pushf\n\t"
        "pop %0"
        : "=r"(eflags)
        : 
        : "cc"
    );
    return eflags;
}