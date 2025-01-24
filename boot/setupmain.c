#include "kernel/x86.h"
#include "kernel/mbr.h"
#include "kernel/fat16.h"
#include "kernel/elf.h"
#include "kernel/memlayout.h"
#include "algobase.h"

#define KERNEL_NAME "kernel"                                // 内核 ELF 文件名（长度不超过 8 字节）
#define ELF ((ELFHeader *)0x8000)                           // 内核 ELF 加载位置
#define SECTSIZE 512                                        // 扇区大小为 512 字节
#define MBR (((MBR *)0x7C00))                               // 指针类型转换，读取位于内存 0x7C00 的 MBR
#define HW_MAP_START_ADDR 0xA0000                           // 前 1MB 可用地址上界，0xA0000 ~ 0xFFFFF 是硬件映射空间
#define TOUPPER(x) (x + ('A' - 'a') * (x > 'a' && x < 'z')) // 字符转换为大写

char buffer[SECTSIZE];        // 临时存储区
char *vmem = (char *)0xB8000; // 显存指针
char vattr = 0x02;            // 默认显示绿色字符

void puts(char *s);
void error(char *s);
void readsect(void *dst, uint32_t offset);
uint32_t data_clus_to_lba28(uint32_t data_fst_sec, uint32_t sec_per_clus, uint32_t cluster);
uint16_t next_clus(uint16_t cluster, uint32_t fat_fst_sec);
uint8_t check_clus(uint16_t cluster);
void memcpy(void *dst, void *src, uint32_t size);
void memset(void *dst, uint8_t value, uint32_t size);

void setupmain(void)
{
    puts("In protected mode.");

    /**
     * 测试 A20 gate
     *
     * A20 gate 未启用会导致地址第 20 位（从 0 开始数）始终为 0
     * 比如地址 0x123456 会变成 0x023456，也就是说 A20 gate 未启用时这两个地址是等价的
     * 判断的方法，就是向这两个地址写入不同的数据，然后再读取数据判断是否不同
     */
    *(uint8_t *)0x123456 = 0x00;
    *(uint8_t *)0x023456 = 0xFF;
    if (*(uint8_t *)0x123456 == *(uint8_t *)0x023456)
    {
        error("A20 gate test failed");
    }

    /**
     * 寻找引导分区
     *
     * MBR 已经被读取到了 0x7C00，因此我们只需要将其转换为 MBR 结构体指针
     * 接着遍历 MBR 分区信息，找到首个可引导分区
     */
    PartitionEntry boot_part = {.boot_indicator = 0};

    // 遍历分区表，找到首个可引导分区
    for (uint32_t i = 0; i < 4; i++)
    {
        if (MBR->partitions[i].boot_indicator == MBR_BOOTABLE_FLAG)
        {
            boot_part = MBR->partitions[i];
            break;
        }
    }

    if (boot_part.boot_indicator != MBR_BOOTABLE_FLAG)
    {
        error("No bootable partition found");
    }

    /**
     * 读取分区文件系统参数
     *
     * 找到引导分区后，先读取该分区的第一个扇区到 buffer
     * 将 buffer 转换为 FAT 引导扇区结构体 FatBootSector 指针，以获取 BPB 和 EBPB
     * 然后验证文件系统类型是否为 FAT16
     */
    readsect(buffer, boot_part.start_lba);
    BPB bpb = ((FatBootSector *)buffer)->bpb;
    EBPB ebpb = ((FatBootSector *)buffer)->ebpb;

    // 判断是否为 FAT16
    for (uint32_t i = 0; i < 5; i++)
    {
        if (ebpb.fs_type[i] != "FAT16"[i])
        {
            error("Partition file system is not FAT16");
        }
    }

    /**
     * 查找内核 ELF 文件条目
     *
     * 我们默认内核保存在引导分区的根目录下
     * 首先根据 BPB 信息计算出 FAT 文件系统各部分的起始位置
     * 接着遍历根目录区域的所有文件条目，检查文件名是否匹配，并记录查找结果
     */

    // 计算 FAT16 各区域起始扇区（相对硬盘起始位置）
    uint32_t const fat_fst_sec = boot_part.start_lba + bpb.rsvd_sec_cnt;                              // FAT 表起始扇区
    uint32_t const root_fst_sec = fat_fst_sec + (bpb.num_fats * bpb.sec_per_fat_16);                  // 根目录起始扇区
    uint32_t const root_sec_cnt = (bpb.root_ent_cnt * sizeof(FatDirEntry) + SECTSIZE - 1) / SECTSIZE; // 根目录占用扇区数量（向上取整）
    uint32_t const data_fst_sec = root_fst_sec + root_sec_cnt;                                        // 数据区起始扇区

    // 搜索对应的根目录下的内核文件条目
    uint8_t is_found = 0;
    FatDirEntry kernel_file_entry;

    for (uint32_t sec_i = 0, entry_i = 0;
         sec_i < root_sec_cnt && !is_found;
         sec_i++)
    {
        readsect(buffer, root_fst_sec + sec_i);

        FatDirEntry const *entry = (FatDirEntry *)buffer;

        for (int32_t lim = SECTSIZE / sizeof(FatDirEntry); // 防止超过 buffer 边界
             lim-- > 0 && entry_i < bpb.root_ent_cnt && !is_found;
             entry_i++, entry++)
        {
            // 判断是否为内核文件
            is_found = 1;

            // FIXME: 这种匹配方式仅会判断前缀是否相同
            // 比如 KERNEL_NAME 为 "kernel" 时，文件名 "kernel", "kernel00", "kernel.bin" 都是合法选项
            // 若要判断是否完全相同，则要判断 KERNEL_NAME 字符串结尾的 '\0' 和 8.3 文件名结尾的空格填充
            for (uint32_t i = 0; KERNEL_NAME[i]; i++)
            {
                // 文件名不区分大小写
                is_found &= (TOUPPER(entry->name[i]) == TOUPPER(KERNEL_NAME[i]));
            }

            if (is_found)
            {
                kernel_file_entry = *entry;
            }
        }
    }

    if (!is_found)
    {
// 由于没有字符串格式化函数，且 error 函数只能输出一次
// 所以，此处使用宏定义来拼接字符串
#define KERNEL_NOT_FOUND_ERROR_MSG "Kernel file \"" KERNEL_NAME "\" not found"

        error(KERNEL_NOT_FOUND_ERROR_MSG);
    }

    /**
     * 读取内核 ELF 文件
     *
     * 有了文件条目之后就可以读取文件数据了
     * 根据簇号计算偏移，读取数据区对应内容，接着在 FAT 表查找下一个簇号
     * 循环读取直到遇见无效簇号
     */
    void *elf_load_ptr = ELF;

    for (uint16_t cluster = kernel_file_entry.fst_clus;
         check_clus(cluster);
         cluster = next_clus(cluster, fat_fst_sec))
    {
        uint32_t offset = data_clus_to_lba28(data_fst_sec, bpb.sec_per_clus, cluster);
        for (uint32_t i = 0; i < bpb.sec_per_clus; i++)
        {
            // 防止写入到硬件映射内存
            if (elf_load_ptr + SECTSIZE - 1 >= HW_MAP_START_ADDR)
            {
                error("Cannot write to hardware mapped address");
            }
            readsect(elf_load_ptr, offset + i);
            elf_load_ptr += SECTSIZE;
        }
    }

    if (ELF->e_magic != ELF_MAGIC)
    {
        error("The ELF header's magic number is incorrect.");
    }

    /**
     * 加载内核程序
     *
     * 上一步只是加载了内核的 ELF 文件到内存之中
     * 接下来要根据 ELF 文件描述，加载程序各个段到指定内存位置
     * 最后跳转到 ELF 记录的入口地址执行内核
     */
    ProgramHeader *prog = (ProgramHeader *)((void *)ELF + ELF->e_phoff);
    uint32_t kernel_start = __UINT32_MAX__, kernel_end = 0;

    for (uint32_t i = 0; i < ELF->e_phnum; i++, prog++)
    {
        if (prog->p_memsz == 0 || prog->p_type != PT_LOAD)
        {
            continue;
        }

        // 记录内核区域的内存范围
        kernel_start = MIN(kernel_start, prog->p_paddr);
        kernel_end = MAX(kernel_end, prog->p_paddr + prog->p_memsz);

        // 将 ELF 文件中的段数据拷贝到内存中
        memcpy((void *)prog->p_paddr, (void *)ELF + prog->p_offset, prog->p_filesz);

        // p_memsz 表示段分配的内存空间，p_filesz 表示段在 ELF 文件中数据大小
        // p_filesz < p_memsz 时，剩余的空间用 0 填充
        memset((void *)prog->p_paddr + prog->p_filesz, 0x0, prog->p_memsz - prog->p_filesz);
    }

    // 写入参数到目标地址
    *(uint32_t *)P_KERNEL_ADDR_START = kernel_start;
    *(uint32_t *)P_KERNEL_ADDR_END = kernel_end;

    // 跳转到内核入口，不会返回
    ((void (*)())(ELF->e_entry))();

    error("The kernel should not return");
}

/* ======================================== */

void putchar(char c)
{
    *(vmem++) = c;     // ASCII
    *(vmem++) = vattr; // 字符属性
}

void puts(char *s)
{
    while (*s)
    {
        putchar(*(s++));
    }
}

/**
 * 输出错误信息并阻塞程序
 */
void error(char *s)
{
    vattr = 0xC; // 显示红色高亮字符
    puts(s);
    while (1)
        ;
}

/* ======================================== */

/**
 * 等待磁盘就绪
 */
void waitdisk(void)
{
    // 检测 BSY 和 RDY 标志
    // FIXME: 检测 DRQ 标志
    while ((inb(0x1F7) & 0xC0) != 0x40)
        /* do nothing */;
}

/**
 * LBA28 寻址读取单个扇区数据
 *
 * @param offset LBA28 起始偏移，以扇区单位
 */
void readsect(void *dst, uint32_t offset)
{
    if (offset > 0x0FFFFFFF)
    {
        error("Offset exceeds LBA28 boundary");
    }

    waitdisk();

    outb(0x1F2, 1); // count = 1
    outb(0x1F3, offset);
    outb(0x1F4, offset >> 8);
    outb(0x1F5, offset >> 16);
    outb(0x1F6, (offset >> 24) | 0xE0);
    outb(0x1F7, 0x20); // cmd 0x20 - read sectors

    waitdisk();

    // 读取扇区
    insl(0x1F0, dst, SECTSIZE / 4);
}

/* ======================================== */

/**
 * 将 FAT16 数据区簇号转换为 LBA28 偏移量
 *
 * @param data_fst_sec 数据区起始扇区（相对于硬盘起始位置）
 */
uint32_t data_clus_to_lba28(uint32_t data_fst_sec, uint32_t sec_per_clus, uint32_t cluster)
{
    // 数据区有效簇号起始值为 2，所以要减去 2 再乘扇区数
    return data_fst_sec + sec_per_clus * (cluster - 2);
}

/**
 * 获取 FAT16 下一个簇号
 *
 * @param cluster 当前簇号（必须为合法值）
 * @param fat_fst_sec FAT 表起始扇区（相对于硬盘起始位置）
 */
uint16_t next_clus(uint16_t cluster, uint32_t fat_fst_sec)
{
    // FAT 表的起始簇号不需要减 2
    uint32_t byte_offset = cluster * 2;

    readsect(buffer, fat_fst_sec + byte_offset / SECTSIZE);
    return *(uint16_t *)(buffer + (byte_offset % SECTSIZE));
}

/**
 * 检验 FAT16 簇号合法性
 */
uint8_t check_clus(uint16_t cluster)
{
    return cluster > 0x1 && cluster < 0xFFF0;
}

/* ======================================== */

void memcpy(void *dst, void *src, uint32_t size)
{
    while (size--)
    {
        *(uint8_t *)(dst++) = *(uint8_t *)(src++);
    }
}

void memset(void *dst, uint8_t value, uint32_t size)
{
    while (size--)
    {
        *(uint8_t *)(dst++) = value;
    }
}
