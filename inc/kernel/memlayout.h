#pragma once

#define HIGHER_HALF_KERNEL_BASE 0xC0000000

#define P_KERNEL_ADDR_START 0x1000 // 存放“内核内存起始地址”参数的地址
#define P_KERNEL_ADDR_END 0x1004   // 存放“内核内存末尾地址”参数的地址