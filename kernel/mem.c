#include "kernel/gdt.h"
#include "kernel/page.h"

void mem_init(void)
{
    gdt_init();
    page_init();
}