#pragma once

#include "types.h"

void tty_init(void);
void tty_clear(void);
int tty_write(const char *buf, size_t n);