#pragma once

#include "types.h"

typedef struct page_node
{
    uint32_t addr;
    size_t count;
    struct page_node *next;
} page_node;

void pmu_add_record(uint32_t addr, size_t count);
page_node *pmu_alloc_pages(size_t count);
void pmu_free_pages(page_node *node);