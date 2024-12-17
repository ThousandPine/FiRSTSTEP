#pragma once

#include "types.h"

static inline uint8_t inb(uint16_t port)
{
    uint8_t value;
    asm volatile("inb %1, %0"
                 : "=a"(value)  // 输出到 AL 寄存器（存储结果）
                 : "Nd"(port)); // 输入端口号（立即数或 DX）
    return value;
}

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

static inline void outb(uint16_t port, uint8_t value)
{
    asm volatile("outb %0, %1"
                 :             // 没有输出操作数
                 : "a"(value), // 输入值，放入 AL
                   "Nd"(port)  // 输入端口号
    );
}