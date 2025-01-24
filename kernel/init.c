#include "kernel/kernel.h"
#include "kernel/x86.h"

void tty_init(void);
void mem_init(void);
void fs_init(void);
void idt_init(void);
void pic_init(void);

#include "kernel/fs.h"
#include "kernel/elf.h"

int init(void)
{
    tty_init();

    mem_init();

    idt_init();
    pic_init();

    fs_init();

    // 测试
    int elf_loader(File * file);
    File file;
    file_open("/kernel", &file);
    elf_loader(&file);

    // 内核不能 return，并且返回地址已经在初始化栈时丢失了
    panic("The kernel runs to the end");
}
