#pragma once

#include "types.h"

#define PAGE_SIZE (1U << 12) // 单个页面大小 4 KiB

typedef struct page_dir_entry
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
} __attribute__((packed)) page_dir_entry;

typedef struct page_tabel_entry
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
} __attribute__((packed)) page_tabel_entry;