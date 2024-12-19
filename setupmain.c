#include "x86.h"
#include "mbr.h"
#include "fat16.h"
#include "elf.h"

#define KERNEL_NAME "kernel"                // 内核 ELF 文件名（长度不超过 8 字节）
#define ELF ((struct ElfHdr *)0x8000)       // 内核 ELF 加载位置
#define SECTSIZE 512                        // 扇区大小为 512 字节
#define MBR (((struct MBR *)0x7C00))        // 指针类型转换，读取位于内存 0x7C00 的 MBR
#define BOOTABLE_FLAG 0x80                  // 引导分区标志

char buffer[SECTSIZE];        // 临时存储区
char *vmem = (char *)0xB8000; // 显存指针
char vattr = 0x02;            // 默认显示绿色字符

char toupper(char c);
void putchar(char c);
void puts(char *s);
void putn(uint32_t n);
void error(char *s);
void waitdisk(void);
void readsect(void *dst, uint32_t offset);

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
 * 判断 FAT16 簇号合法性
 */
uint8_t check_clus(uint16_t cluster)
{
    return cluster > 0x1 && cluster < 0xFFF0;
}

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

void setupmain(void)
{
    puts("In protected mode.");

    struct PartitionEntry boot_part = {.boot_indicator = 0};

    // 读取位于内存 0x7C00 的 MBR 分区表
    // 找到首个可引导分区
    for (uint32_t i = 0; i < 4; i++)
    {
        if (MBR->partitions[i].boot_indicator == BOOTABLE_FLAG)
        {
            boot_part = MBR->partitions[i];
            break;
        }
    }

    if (boot_part.boot_indicator != BOOTABLE_FLAG)
    {
        error("No bootable partition found");
    }

    // 读取 FAT 分区信息
    readsect(buffer, boot_part.start_lba);
    struct BPB bpb = ((struct FatBootSector *)buffer)->bpb;
    struct EBPB ebpb = ((struct FatBootSector *)buffer)->ebpb;

    // 判断是否为 FAT16
    for (uint32_t i = 0; i < 5; i++)
    {
        if (ebpb.fs_type[i] != "FAT16"[i])
        {
            error("Partition file system is not FAT16");
        }
    }

    // 计算 FAT16 各区域起始扇区（相对硬盘起始位置）
    uint32_t fat_fst_sec = boot_part.start_lba + bpb.rsvd_sec_cnt;                                     // FAT 表起始扇区
    uint32_t root_fst_sec = fat_fst_sec + (bpb.num_fats * bpb.sec_per_fat_16);                         // 根目录起始扇区
    uint32_t root_sec_cnt = (bpb.root_ent_cnt * sizeof(struct FatDirEntry) + SECTSIZE - 1) / SECTSIZE; // 根目录占用扇区数量（向上取整）
    uint32_t data_fst_sec = root_fst_sec + root_sec_cnt;                                               // 数据区起始扇区

    // 搜索对应的根目录下的内核文件条目
    struct FatDirEntry kernel_file_entry;
    uint8_t kernel_found = 0;

    for (uint32_t sec_i = 0, entry_i = 0;
         sec_i < root_sec_cnt && !kernel_found;
         sec_i++)
    {
        readsect(buffer, root_fst_sec + sec_i);

        struct FatDirEntry const *entry = (struct FatDirEntry *)buffer;

        for (int32_t lim = SECTSIZE / sizeof(struct FatDirEntry); // 防止超过 buffer 边界
             lim-- > 0 && entry_i < bpb.root_ent_cnt && !kernel_found;
             entry_i++, entry++)
        {
            // 判断是否为内核文件
            kernel_found = 1;

            // FIXME: 这种匹配方式仅会判断前缀是否相同
            // 比如 KERNEL_NAME 为 "kernel" 时，文件名 "kernel", "kernel00", "kernel.bin" 都是合法选项
            // 若要判断是否完全相同，则要判断 KERNEL_NAME 字符串结尾的 '\0' 和 8.3 文件名结尾的空格填充
            for (uint32_t i = 0; KERNEL_NAME[i]; i++)
            {
                // 文件名不区分大小写
                kernel_found &= (toupper(entry->name[i]) == toupper(KERNEL_NAME[i]));
            }

            if (kernel_found)
            {
                kernel_file_entry = *entry;
            }
        }
    }

    if (!kernel_found)
    {
// 由于没有字符串格式化函数，且 error 函数只能输出一次
// 所以，此处使用宏定义来拼接字符串
#define KERNEL_NOT_FOUND_ERROR_MSG "Kernel file \"" KERNEL_NAME "\" not found"

        error(KERNEL_NOT_FOUND_ERROR_MSG);
    }

    // 读取内核 ELF 到指定内存地址
    // FIXME: 防止覆盖到高位的映射内存
    void *elf_load_ptr = ELF;

    for (uint16_t cluster = kernel_file_entry.fst_clus;
         check_clus(cluster);
         cluster = next_clus(cluster, fat_fst_sec))
    {
        uint32_t offset = data_clus_to_lba28(data_fst_sec, bpb.sec_per_clus, cluster);
        for (uint32_t i = 0; i < bpb.sec_per_clus; i++)
        {
            readsect(elf_load_ptr, offset + i);
            elf_load_ptr += SECTSIZE;
        }
    } 

    if (ELF->e_magic != ELF_MAGIC)
    {
        error("The ELF header's magic number is incorrect.");
    }

    // 根据 ELF 文件描述，加载程序各个段到指定内存位置
    struct ProgHdr *prog_hdr = (struct ProgHdr *) ((void *) ELF + ELF->e_phoff);

    for (uint32_t i = 0; i < ELF->e_phnum; i++, prog_hdr++)
    {
        // 将 ELF 文件中的程序段数据拷贝到内存中
        memcpy((void *)prog_hdr->p_paddr, (void *)ELF + prog_hdr->p_offset, prog_hdr->p_filesz);

        // p_memsz 表示段分配的内存空间，p_filesz 表示段在 ELF 文件中数据大小
        // p_filesz < p_memsz 时，剩余的空间用 0 填充
        memset((void *)prog_hdr->p_paddr + prog_hdr->p_filesz, 0x0, prog_hdr->p_memsz - prog_hdr->p_filesz);
    }
    
    // 跳转到内核入口，不会再返回
    ((void (*)(void)) (ELF->e_entry))();

    error("The kernel should not return");
}

char toupper(char c)
{
    return c + ('A' - 'a') * (c > 'a' && c < 'z');
}

void putchar(char c)
{
    *(vmem++) = c;
    *(vmem++) = vattr;
}

void puts(char *s)
{
    while (*s)
    {
        putchar(*(s++));
    }
}

void putn(uint32_t n)
{
    uint32_t t = 0;
    uint32_t x = 0;

    do
    {
        x *= 10;
        x += n % 10;
        n /= 10;
        ++t;
    } while (n);

    while (t--)
    {
        putchar((x % 10) + '0');
        x /= 10;
    }
}

void error(char *s)
{
    vattr = 0xC; // 显示红色高亮字符
    puts(s);
    while (1)
        /* do nothing */;
}

void waitdisk(void)
{
    // 等待硬盘就绪
    while ((inb(0x1F7) & 0xC0) != 0x40)
        /* do nothing */;
}

/**
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