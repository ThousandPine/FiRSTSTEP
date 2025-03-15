#include "kernel/elf.h"
#include "kernel/fs.h"
#include "kernel/kernel.h"
#include "kernel/pmu.h"
#include "kernel/page.h"
#include "algobase.h"
#include "string.h"

/**
 * 将 ELF 文件中的程序加载到内存中
 *
 * FIXME: 补充文件读取失败的情况
 * FIXME: 错误提前退出时，要释放申请的资源（比如页目录、页表和页）
 * TODO: 将程序相关的信息（页目录、程序入口）保存到类似 PCB 的结构体之中
 * 
 * @param elf ELF 文件
 * @return 程序入口的虚拟地址，0 表示加载失败
 */
uint32_t elf_loader(page_dir_entry *user_page_dir, const char *file_path)
{
    file_struct elf;
    if (file_open(file_path, &elf) != 0)
    {
        DEBUGK("Open ELF file failed");
        return 0;
    }

    elf_header elfhdr;

    // 读取 ELF 文件头
    file_read(&elfhdr, 0, sizeof(elfhdr), &elf);
    // 校验魔数
    if (elfhdr.e_magic != ELF_MAGIC)
    {
        DEBUGK("ELF file verification failed");
        return 0;
    }

    // 加载程序段
    for (size_t i = 0; i < elfhdr.e_phnum; i++)
    {
        // 读取程序头
        program_header ph;
        file_read(&ph, elfhdr.e_phoff + sizeof(program_header) * i, sizeof(program_header), &elf);

        DEBUGK("Program Header [v_addr: %p] [filesz: %#x] [memsz: %#x] [type: %#x] [flags: %#x]", ph.p_vaddr, ph.p_filesz, ph.p_memsz, ph.p_type, ph.p_flags);

        // 是否可加载
        if (ph.p_type != PT_LOAD)
        {
            continue;
        }

        // 以页为跨度，写入程序段的内存数据
        size_t write_bytes = 0; // 记录已写入的数据量
        for (uint32_t v_addr = ph.p_vaddr, v_end = ph.p_vaddr + ph.p_memsz; v_addr < v_end;)
        {
            // 申请页内存并映射到线性地址
            uint32_t p_addr = pmu_alloc();
            map_physical_page_to_linear(user_page_dir, p_addr, v_addr, 1, ph.p_flags & PF_W);


            // 写入页内部分的数据
            off_t offset = v_addr % PAGE_SIZE;                           // 计算页内偏移
            size_t write_size = MIN(v_end - v_addr, PAGE_SIZE - offset); // 计算页内写入数据大小
            // 写入文件包含的部分
            if (write_bytes < ph.p_filesz)
            {
                // 计算需要从文件中读取的数据量
                size_t read_filesz = MIN(write_size, ph.p_filesz - write_bytes);
                file_read((void *)p_addr + offset, ph.p_offset + write_bytes, read_filesz, &elf);

                offset += read_filesz;
                write_size -= read_filesz;
                write_bytes += read_filesz;
            }
            // 写入文件不包含的部分（清 0）
            memset((void *)p_addr + offset, 0, write_size);
            write_bytes += write_size;

            // 移动指针到下一个页开头
            v_addr = ALIGN_DOWN(v_addr, PAGE_SIZE) + PAGE_SIZE;
        }
    }

    return elfhdr.e_entry;
}