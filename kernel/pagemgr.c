#include "kernel/pagemgr.h"
#include "kernel/page.h"
#include "kernel/kernel.h"
#include "kernel/x86.h"

#define NODE_COUNT 1024 // 链表节点缓冲区大小

static page_node node_buf[NODE_COUNT];             // 链表节点缓冲区
static uint8_t bitmap[(NODE_COUNT + 7) / 8] = {0}; // 位图初始化为全 0

static struct
{
    size_t count;
    page_node *head;
} pagemgr = {.count = 0, .head = NULL};

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
static void free_node(struct page_node *node)
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
void pagemgr_add_record(uint32_t addr, size_t count)
{
    DEBUGK("Add page free record: addr = %p, count = %u", addr, count);

    assert(count != 0);          // 数量不得为 0
    assert((addr & 0xFFF) == 0); // 地址 4 KiB 对齐

    pagemgr.count += count;

    if (pagemgr.head == NULL)
    {
        pagemgr.head = alloc_node(addr, count);
        return;
    }

    // 找到插位置的前/后节点，第一个地址比 addr 大的节点为后置节点
    page_node *p = NULL, *p_pre = NULL, *p_next = pagemgr.head;
    while (p_next != NULL && addr > p_next->addr)
    {
        p_pre = p_next;
        p_next = p_next->next;
    }

    // 不能与前后节点的内存范围相交
    assert(p_pre == NULL || p_pre->addr + p_pre->count * PAGE_SIZE <= addr);
    assert(p_next == NULL || addr + count * PAGE_SIZE <= p_next->addr);

    // 判断能否与后一节点合并
    if (p_pre != NULL && p_pre->addr + p_pre->count * PAGE_SIZE == addr)
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
            pagemgr.head = p;
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
 * 申请指定数量的内存页
 *
 * @return 内存页列表，可能包括多个节点,内存不足时返回 NULL;
 *         请保存该链表，用于后续释放。
 */
page_node *pagemgr_alloc(size_t count)
{
    if (pagemgr.count < count)
    {
        return NULL;
    }

    // 当前头节点就是结果链表的头节点
    page_node *result = pagemgr.head;

    // 从第一个节点开始累加，找到满足 count 数量的最后一个节点
    page_node *p = pagemgr.head;
    size_t sum = p->count;
    while (sum < count)
    {
        p = p->next;
        sum += p->count;
    }

    // 空闲页面数量大于等于 count 的情况下 p 不可能为 NULL
    assert(p != NULL);

    // sum 和 count 的差值就是最后一个节点多余的页面
    size_t surplus = sum - count;
    size_t require = p->count - surplus;

    // 该节点有剩余的页面，则将节点拆分成两个部分
    if (surplus > 0)
    {
        // 创建新节点保存节点中多余的页面，并作为新的头节点
        pagemgr.head = alloc_node(p->addr + require * PAGE_SIZE, surplus);
        pagemgr.head->next = p->next;
        // 取走的节点则只保留需要的数量
        p->count = require;
    }
    // 无剩余页面，表示把整个节点取走
    else
    {
        // 则将下一个节点当作新的头节点
        pagemgr.head = p->next;
    }
    // 断开与原链表的连接
    p->next = NULL;

    return result;
}

/**
 * 将链表与其中记录的页面设置为空闲
 *
 * @param node 链表需要来自 pagemgr_alloc 的返回值
 */
void pagemgr_free(page_node *node)
{
    while (node != NULL)
    {
        uint32_t addr = node->addr;
        size_t count = node->count;
        page_node *next = node->next;

        free_node(node);                 // 释放节点
        pagemgr_add_record(addr, count); // 添加空闲页记录
        node = next;
    }
}