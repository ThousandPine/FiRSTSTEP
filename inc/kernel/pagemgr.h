#pragma once

#include "types.h"

typedef struct page_node
{
    uint32_t addr;
    size_t count;
    struct page_node *next;
} page_node;

void pagemgr_add_record(uint32_t addr, size_t count);
page_node *pagemgr_alloc(size_t count);
void pagemgr_free(page_node *node);