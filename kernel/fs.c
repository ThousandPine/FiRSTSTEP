#include "kernel/fs.h"
#include "kernel/ata.h"
#include "kernel/mbr.h"
#include "kernel/fat16.h"
#include "kernel/kernel.h"

#define FILENAME_MAX_LENGTH 12 // 文件名最大长度
#define BLANK ' '              // 填充符号为空格
#define PATH_SEPARATOR '/'     // 路径分隔符

static uint8_t buf[SECT_SIZE];
static PartitionEntry part = {0};
static struct
{
    lba_t fat_start_lba;       // FAT 表起始扇区（LBA 格式）
    lba_t root_start_lba;      // 根目录起始扇区（LBA 格式）
    lba_t data_start_lba;      // 数据区起始扇区（LBA 格式）
    uint32_t root_num_sectors; // 根目录占用扇区数量（向上取整）
    BPB bpb;
} fat;

static inline char toupper(char c)
{
    return (c >= 'a' && c <= 'z') ? (c - ('a' - 'A')) : c;
}

/**
 * 将 path 的下一个文件名写入到 buf
 * 会在 buf 末尾添加 '\0'
 * 会修改 path 指向下个文件名之后的位置
 */
static void next_name(const char **path, char *buf)
{
    while (**path == PATH_SEPARATOR)
    {
        ++(*path);
    }

    for (int i = 0; **path && **path != PATH_SEPARATOR;)
    {
        assert(++i <= FILENAME_MAX_LENGTH);
        *(buf++) = *((*path)++);
    }
    *buf = '\0';
}

/**
 * 比较 8.3 文件名
 *
 * @param name 8.3 文件名字符串，扩展名前需要包括 '.'
 * @return 0 匹配成功，-1 匹配失败
 */
static int fat_name_cmp(const FatDirEntry *entry, const char *name)
{
    // 计算文件名长度，除去结尾填充的空格
    size_t namel = sizeof(entry->name);
    while (namel > 0 && entry->name[namel - 1] == BLANK)
    {
        --namel;
    }
    // 计算扩展名长度，除去结尾填充的空格
    size_t extl = sizeof(entry->ext);
    while (extl > 0 && entry->ext[extl - 1] == BLANK)
    {
        --extl;
    }

    // 比较文件名
    for (size_t i = 0; i < namel; i++)
    {
        if (toupper(entry->name[i]) != toupper(name[i]))
        {
            return -1;
        }
    }
    name += namel;

    if (extl == 0)
    {
        return *name == '\0' ? 0 : -1;
    }

    // 比较拓展名
    if (*(name++) != '.')
    {
        return -1;
    }
    for (size_t i = 0; i < extl; i++)
    {
        if (toupper(entry->ext[i]) != toupper(name[i]))
        {
            return -1;
        }
    }
    name += extl;

    return *name == '\0' ? 0 : -1;
}

/**
 * 获取 FAT16 下一个簇号
 *
 * @param cluster 当前簇号（必须为合法值）
 */
uint16_t fat_next_clus(uint16_t cluster)
{
    // FAT 表的有效起始簇号是 0 ，所以不需要减 2
    uint32_t byte_offset = cluster * 2;

    ata_read(buf, fat.fat_start_lba + byte_offset / SECT_SIZE, 1);
    return *(uint16_t *)(buf + (byte_offset % SECT_SIZE));
}

// 检验 FAT16 簇号合法性
uint8_t fat_check_clus(uint16_t cluster)
{
    return cluster > 0x1 && cluster < 0xFFF0;
}

/**
 * 将文件数据偏移量转换为 LBA 地址
 *
 * @param offset 文件内部偏移量，必须是扇区大小的整数倍
 * @return LBA 地址，大于 0 为有效值，0 表示失败
 */
static lba_t fat_off2lba(const FatDirEntry *entry, off_t offset)
{
    assert(offset % SECT_SIZE == 0);

    // 找到所在的簇号
    const size_t clus_size = fat.bpb.sec_per_clus * SECT_SIZE;
    uint16_t cur_clus = entry->fst_clus;
    while (offset >= clus_size)
    {
        cur_clus = fat_next_clus(cur_clus);
        offset -= clus_size;
    }

    if (!fat_check_clus(cur_clus))
    {
        return 0;
    }

    /**
     * 计算簇号对应的 LBA 地址
     * 数据区有效簇号起始值为 2，所以要减去 2 再乘扇区数
     * 最后加上簇内 offset 的偏移扇区数
     */
    lba_t lba = fat.data_start_lba + fat.bpb.sec_per_clus * (cur_clus - 2) + offset / SECT_SIZE;
    // 理论上来说文件内部 LBA 地址不可能为 0
    assert(lba > 0);
    return lba;
}

/**
 * 在 FAT 文件系统搜索文件条目
 *
 * @param path 文件绝对路径
 * @param out_entry 保存找到的条目信息
 * @return 0 成功，-1 失败
 */
static int fat_find_entry(const char *path, FatDirEntry *out_entry)
{
    // 必须使用绝对路径
    assert(path[0] == PATH_SEPARATOR);

    uint8_t find_flag = 0;
    FatDirEntry cur_entry = {0};

    // 获得第一个文件名
    static char namebuf[FILENAME_MAX_LENGTH + 1];
    next_name(&path, namebuf);

    // 遍历根目录区域查找文件条目
    for (uint32_t i = 0; !find_flag && i < fat.root_num_sectors; i++)
    {
        const FatDirEntry *entries = (const FatDirEntry *)buf;
        ata_read(buf, fat.root_start_lba + i, 1);

        for (int t = SECT_SIZE / sizeof(FatDirEntry); !find_flag && t--; entries++)
        {
            // 表示该条目及后续条目都为空
            if (entries->name[0] == 0)
            {
                break;
            }
            // 比较文件名
            if (fat_name_cmp(entries, namebuf) == 0)
            {
                cur_entry = *entries;
                find_flag = 1;
            }
        }
    }

    if (!find_flag)
    {
        DEBUGK("Cannot found \"%s\" in root", namebuf);
        return -1;
    }

    // 在子目录中查找后续文件
    while (*path != '\0')
    {
        // 获得下一个文件名
        next_name(&path, namebuf);
        if (namebuf[0] == '\0')
        {
            break;
        }

        // 判断当前条目是否是目录
        if ((cur_entry.attr & FAT_ATTR_DIRECTORY) == 0)
        {
            DEBUGK("\"%s\" is not a directory", namebuf);
            return -1;
        }

        // 遍历当前条目的数据区，查找同名的文件条目
        find_flag = 0;
        for (off_t offset = 0; !find_flag; offset += SECT_SIZE)
        {
            const FatDirEntry *entries = (const FatDirEntry *)buf;
            lba_t lba = fat_off2lba(&cur_entry, offset);

            /**
             * 由于目录的 file_size 始终为 0
             * 不能直接使用 offset < file_size 判断是否读取完毕
             * 所以这里使用 fat_off2lba 的返回值判断是否到达文件末尾
             */
            if (lba == 0)
            {
                break;
            }
            ata_read(buf, lba, 1);

            for (int t = SECT_SIZE / sizeof(FatDirEntry); !find_flag && t--; entries++)
            {
                // 表示该条目及后续条目都为空
                if (entries->name[0] == 0)
                {
                    break;
                }
                // 比较文件名
                if (fat_name_cmp(entries, namebuf) == 0)
                {
                    // 将当前条目设为找到的条目
                    cur_entry = *entries;
                    find_flag = 1;
                }
            }
        }

        if (!find_flag)
        {
            DEBUGK("Cannot found \"%s\" in \"%s\"", namebuf, cur_entry.name);
            return -1;
        }
    }

    *out_entry = cur_entry;
    return 0;
}

/**
 * 打开文件
 *
 * @param path 文件绝对路径
 * @param out_file 保存文件信息
 * @return 0 成功，-1 失败
 */
int file_open(const char *path, File *out_file)
{
    return fat_find_entry(path, &out_file->fat_entry);
}

void fs_init(void)
{
    // 遍历 MBR 分区表，找到首个引导分区作为文件系统所在分区
    const MBR *mbr = (const MBR *)buf;

    ata_read(buf, 0, 1);
    for (uint32_t i = 0; i < 4; i++)
    {
        if (mbr->partitions[i].boot_indicator == MBR_BOOTABLE_FLAG)
        {
            part = mbr->partitions[i];
            break;
        }
    }
    assert(part.boot_indicator & MBR_BOOTABLE_FLAG);

    /**
     * 读取分区文件系统参数
     *
     * 找到引导分区后，先读取该分区的第一个扇区到 buffer
     * 将 buffer 转换为 FAT 引导扇区结构体 FatBootSector 指针，以获取 BPB 和 EBPB
     * 然后验证文件系统类型是否为 FAT16
     */
    const FatBootSector *fbs = (const FatBootSector *)buf;
    ata_read(buf, part.start_lba, 1);
    fat.bpb = fbs->bpb;

    // 判断是否为 FAT16
    for (uint32_t i = 0; i < 5; i++)
    {
        if (fbs->ebpb.fs_type[i] != "FAT16"[i])
        {
            panic("Partition file system is not FAT16");
        }
    }

    // 计算 FAT16 各区域参数
    fat.fat_start_lba = part.start_lba + fat.bpb.rsvd_sec_cnt;                                       // FAT 表起始扇区
    fat.root_start_lba = fat.fat_start_lba + (fat.bpb.num_fats * fat.bpb.sec_per_fat_16);            // 根目录起始扇区
    fat.root_num_sectors = (fat.bpb.root_ent_cnt * sizeof(FatDirEntry) + SECT_SIZE - 1) / SECT_SIZE; // 根目录占用扇区数量（向上取整）
    fat.data_start_lba = fat.root_start_lba + fat.root_num_sectors;                                  // 数据区起始扇区
}