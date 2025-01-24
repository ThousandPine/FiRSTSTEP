#pragma once

#include "types.h"

#define PAGE_SIZE (1U << 12) // 单个页面大小 4 KiB

#define page_dir_set(entry, _addr, _us, _rw, _present) \
    do                                                 \
    {                                                  \
        entry.addr = (_addr) >> 12;                    \
        entry.us = (_us);                              \
        entry.rw = (_rw);                              \
        entry.present = (_present);                    \
        entry.pwt = 0;                                 \
        entry.pcd = 0;                                 \
        entry.accessed = 0;                            \
        entry.dirty = 0;                               \
        entry.ps = 0;                                  \
        entry.global = 0;                              \
        entry.avl = 0;                                 \
    } while (0)

#define page_tabel_set(entry, _addr, _us, _rw, _present) \
    do                                                   \
    {                                                    \
        entry.addr = (_addr) >> 12;                      \
        entry.us = (_us);                                \
        entry.rw = (_rw);                                \
        entry.present = (_present);                      \
        entry.pwt = 0;                                   \
        entry.pcd = 0;                                   \
        entry.accessed = 0;                              \
        entry.dirty = 0;                                 \
        entry.pat = 0;                                   \
        entry.global = 0;                                \
        entry.avl = 0;                                   \
    } while (0)

typedef struct PageDirEntry
{
    uint8_t present : 1;  // 是否存在于物理内存
    uint8_t rw : 1;       // 0 只读，1 读写
    uint8_t us : 1;       // 表示权限，0 管理员页，1 用户页
    uint8_t pwt : 1;      // 启用 Write-Through 功能
    uint8_t pcd : 1;      // 禁用页面缓存
    uint8_t accessed : 1; // 是否访问过该条目
    uint8_t dirty : 1;    // 页面是否已被写入
    uint8_t ps : 1;       // 页面大小，4 KiB 或 4 MiB
    uint8_t global : 1;   // 全局页面
    uint8_t avl : 3;      // 保留字段
    uint32_t addr : 20;
} __attribute__((packed)) PageDirEntry;

typedef struct PageTabelEntry
{
    uint8_t present : 1;  // 是否存在于物理内存
    uint8_t rw : 1;       // 0 只读，1 读写
    uint8_t us : 1;       // 表示权限，0 管理员页，1 用户页
    uint8_t pwt : 1;      // 启用 Write-Through 功能
    uint8_t pcd : 1;      // 禁用页面缓存
    uint8_t accessed : 1; // 是否访问过该条目
    uint8_t dirty : 1;    // 页面是否已被写入
    uint8_t pat : 1;      // 控制缓存行为，与 pcd, pwt 相关
    uint8_t global : 1;   // 全局页面
    uint8_t avl : 3;      // 保留字段
    uint32_t addr : 20;
} __attribute__((packed)) PageTabelEntry;