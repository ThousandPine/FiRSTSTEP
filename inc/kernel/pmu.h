#pragma once

#include "types.h"

void pmu_init(uint32_t addr, size_t count);
uint32_t pmu_alloc(void);
void pmu_free(uint32_t addr);