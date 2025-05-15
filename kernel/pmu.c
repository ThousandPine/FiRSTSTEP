#include "kernel/pmu.h"
#include "kernel/page.h"
#include "kernel/kernel.h"
#include "kernel/x86.h"
#include "string.h"

#define NODE_COUNT 1024 // 链表节点缓冲区大小

typedef struct page_node
{
    uint32_t addr;
    size_t count;
    struct page_node *next;
} page_node;

static page_node node_buf[NODE_COUNT];             // 链表节点缓冲区
static uint8_t bitmap[(NODE_COUNT + 7) / 8] = {0}; // 位图初始化为全 0

static struct
{
    size_t count;
    page_node *head;
} pmu = {.count = 0, .head = NULL};

// 检查位图中的某一位是否为 1
static inline int is_bit_set(int index)
{
    return (bitmap[index / 8] >> (index % 8)) & 1;
}

// 设置位图中的某一位为 1
static inline void set_bit(int index)
{
    bitmap[index / 8] |= (1 << (index % 8));
}

// 清除位图中的某一位（设为 0）
static inline void clear_bit(int index)
{
    bitmap[index / 8] &= ~(1 << (index % 8));
}

// 从缓冲区获取空闲链表节点并初始化
static page_node *alloc_node(uint32_t addr, size_t count)
{
    for (int i = 0; i < NODE_COUNT; i++)
    {
        if (!is_bit_set(i))
        {
            set_bit(i);
            node_buf[i].addr = addr;
            node_buf[i].count = count;
            node_buf[i].next = NULL;
            return node_buf + i;
        }
    }
    panic("page node buffer exhausted");
    return NULL;
}

// 释放链表节点
static void free_node(page_node *node)
{
    // 判断节点是否属于 node_buf
    assert(node >= node_buf);
    assert(node <= node_buf + NODE_COUNT - 1);

    clear_bit(node - node_buf);
    node->count = 0;
    node->addr = 0;
    node->next = NULL;
}

// 添加空闲页记录
static void pmu_add_record(uint32_t addr, size_t count)
{
    // DEBUGK("Add page free record: addr = %p, count = %u", addr, count);

    assert(count != 0);          // 数量不得为 0
    assert(addr != 0);           // 地址不得为 0
    assert((addr & 0xFFF) == 0); // 地址 4 KiB 对齐

    pmu.count += count;

    if (pmu.head == NULL)
    {
        pmu.head = alloc_node(addr, count);
        return;
    }

    // 找到插位置的前/后节点，第一个地址比 addr 大的节点为后置节点
    page_node *p = NULL, *p_pre = NULL, *p_next = pmu.head;
    while (p_next != NULL && addr > p_next->addr)
    {
        p_pre = p_next;
        p_next = p_next->next;
    }

    // 不能与前后节点的内存范围相交
    assert(p_pre == NULL || p_pre->addr + p_pre->count * PAGE_SIZE <= addr);
    assert(p_next == NULL || addr + count * PAGE_SIZE <= p_next->addr);

    // 判断能否与后一节点合并
    if (p_next != NULL && addr + PAGE_SIZE == p_next->addr)
    {
        // 合并到后一节点记录中
        p_next->addr = addr;
        p_next->count += count;
        p = p_next;            // 将合并的节点记录为当前节点
        p_next = p_next->next; // 更新后一节点指针
    }
    // 否则先插入一个新节点
    else
    {
        p = alloc_node(addr, count);
        p->next = p_next;
        if (p_pre == NULL)
        {
            pmu.head = p;
        }
        else
        {
            p_pre->next = p;
        }
    }

    // 判断当前节点能否与前一节点合并
    if (p_pre != NULL && p_pre->addr + p_pre->count * PAGE_SIZE == addr)
    {
        p_pre->count += p->count; // 合并页面数量
        free_node(p);             // 释放当前节点
        p_pre->next = p_next;     // 修改下一节点指向
    }
}

/**
 * 申请一个内存页
 *
 * @return 页起始地址，0 表示失败
 */
uint32_t pmu_alloc(void)
{
    if (pmu.count == 0)
    {
        panic("No free page");
        return 0;
    }

    assert(pmu.head != NULL);

    // 从第一个节点取出一页
    uint32_t addr = pmu.head->addr;
    pmu.head->addr += PAGE_SIZE;
    --pmu.count;
    --pmu.head->count;

    // 如果节点被取空，则删除该节点
    if (pmu.head->count == 0)
    {
        page_node *next = pmu.head->next;
        free_node(pmu.head);
        pmu.head = next;
    }

    return addr;
}

/**
 * 释放内存页
 *
 * @param addr 页地址，不得为 0
 */
void pmu_free(uint32_t addr)
{
    if (addr == 0)
    {
        return;
    }
    pmu_add_record(addr, 1);
}

/**
 * 初始化内存页管理器
 *
 * @param addr 空页起始地址
 * @param count 空闲页数量
 */
void pmu_init(uint32_t addr, size_t count)
{
    pmu.count = 0;
    while (pmu.head != NULL)
    {
        page_node *next = pmu.head->next;
        free_node(pmu.head);
        pmu.head = next;
    }
    memset(bitmap, 0, sizeof(bitmap));
    pmu_add_record(addr, count);
}