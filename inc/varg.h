#pragma once

#include "types.h"

// 可变参数的本质是直接通过指针访问栈数据
typedef int8_t *va_list;

// 不同模式下每次 push 的数据大小
#if defined(__i386__) || defined(_M_IX86)
#define STACK_PUSH_SIZE 4
#elif defined(__x86_64__) || defined(_M_X64)
#define STACK_PUSH_SIZE 8 // 只是为了让 IDE 不报错，暂时没有兼容 64 位的计划
#endif

/**
 * 计算类型在栈中占用的空间
 *
 * CPU 每次 push 的字节大小都是固定的
 * 16, 32, 64 位模式下每次 push 的数据大小分别是 2, 4, 8 字节
 *
 * 因此，数据在栈中占用空间不一定等于类型大小，而是要根据 push 的最小单位向上取整
 */
#define va_rounded_size(type) \
    ((sizeof(type) + STACK_PUSH_SIZE - 1) / STACK_PUSH_SIZE * STACK_PUSH_SIZE)

/**
 * 跳过开头的 fmt 参数
 *
 * 函数参数按从右到左顺序入栈，而栈则是向低地址增长
 * 最左侧的 fmt 参数最后入栈，且在地址最低
 *
 * 因此，想要获取调用栈中 fmt 下一个参数的地址
 * 只需要在 fmt 地址基础上加上 fmt 在栈中大小的偏移即可
 */
#define va_start(fmt) ((va_list)(&fmt) + va_rounded_size(fmt))

/**
 * 读取下一个参数并移位指针
 *
 * 等价于以下伪函数：
 * type va_arg<type>(va_list &args)
 * {
 *      type val = *(type *)args;   // 通过指针获取对应类型参数
 *      args += rounded_size(type); // 移动指针到下一个参数
 *      return val;
 * }
 * 上面的代码用到了泛型和引用，这里通过一些 trick 在 C 语言实现等价的功能
 * 1. 通过宏定义代替泛型
 * 2. 借助逗号表达式返回末尾表达式值的特性，免除了中间变量，使其能用宏实现
 */
#define va_arg(args, type) \
    ((args) += va_rounded_size(type), *(type *)((args) - va_rounded_size(type)))


/**
 * 释放资源？
 * 
 * void va_end (va_list) 是 gnulib 中定义的函数
 * 可变参数的标准写法是，使用完 va_list 之后调用 va_end 函数进行“释放”
 * 暂时不知道其具体作用，就先用宏定义占位吧
 */
#define va_end(args) ((args) = (va_list)0)
