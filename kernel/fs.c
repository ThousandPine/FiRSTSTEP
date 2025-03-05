#include "kernel/fs.h"
#include "kernel/ata.h"
#include "kernel/mbr.h"
#include "kernel/fat16.h"
#include "kernel/kernel.h"
#include "algobase.h"
#include "string.h"

#define FILENAME_MAX_LENGTH 12 // 文件名最大长度
#define BLANK ' '              // 填充符号为空格
#define PATH_SEPARATOR '/'     // 路径分隔符

static partition_entry part = {0};
static struct
{
    lba_t fat_start_lba;       // FAT 表起始扇区（LBA 格式）
    lba_t root_start_lba;      // 根目录起始扇区（LBA 格式）
    lba_t data_start_lba;      // 数据区起始扇区（LBA 格式）
    uint32_t root_num_sectors; // 根目录占用扇区数量（向上取整）
    bpb_struct bpb;
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
static int fat_name_cmp(const fat_dir_entry *entry, const char *name)
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
static uint16_t fat_next_clus(uint16_t cluster)
{
    static uint8_t buf[SECT_SIZE];

    // FAT 表的有效起始簇号是 0 ，所以不需要减 2
    uint32_t byte_offset = cluster * 2;

    ata_read(buf, fat.fat_start_lba + byte_offset / SECT_SIZE, 1);
    return *(uint16_t *)(buf + (byte_offset % SECT_SIZE));
}

// 检验 FAT16 簇号合法性
static inline uint8_t fat_check_clus(uint16_t cluster)
{
    return cluster > 0x1 && cluster < 0xFFF0;
}

// 将簇号转换为数据区对应的 LBA 地址
static inline lba_t fat_clus2lba(uint16_t clus)
{
    /**
     * 计算簇号对应的 LBA 地址
     * 数据区有效簇号起始值为 2，所以要减去 2 再乘扇区数
     */
    return fat.data_start_lba + fat.bpb.sec_per_clus * (clus - 2);
}

/**
 * 将文件数据偏移量转换为 LBA 地址
 *
 * @param offset 文件内部偏移量，必须是扇区大小的整数倍
 * @return LBA 地址，大于 0 为有效值，0 表示失败
 */
static lba_t fat_off2lba(const fat_dir_entry *entry, off_t offset)
{
    assert(offset % SECT_SIZE == 0);

    /**
     * 找到 offset 所在的簇号
     * 期间不断减小 offset 的值，将其转换为簇内偏移
     */
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

    // 计算簇号对应的 LBA 地址, 再加上簇内偏移扇区数
    lba_t lba = fat_clus2lba(cur_clus) + offset / SECT_SIZE;
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
static int fat_find_entry(const char *path, fat_dir_entry *out_entry)
{
    static uint8_t buf[SECT_SIZE];

    // 必须使用绝对路径
    assert(path[0] == PATH_SEPARATOR);

    uint8_t find_flag = 0;
    fat_dir_entry cur_entry = {0};

    // 获得第一个文件名
    static char namebuf[FILENAME_MAX_LENGTH + 1];
    next_name(&path, namebuf);

    // 遍历根目录区域查找文件条目
    for (uint32_t i = 0; !find_flag && i < fat.root_num_sectors; i++)
    {
        const fat_dir_entry *entries = (const fat_dir_entry *)buf;
        ata_read(buf, fat.root_start_lba + i, 1);

        for (int t = SECT_SIZE / sizeof(fat_dir_entry); !find_flag && t--; entries++)
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
            const fat_dir_entry *entries = (const fat_dir_entry *)buf;
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

            for (int t = SECT_SIZE / sizeof(fat_dir_entry); !find_flag && t--; entries++)
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
int file_open(const char *path, file_struct *out_file)
{
    if (path == NULL || out_file == NULL)
    {
        return -1;
    }
    return fat_find_entry(path, &out_file->fat_entry);
}

/**
 * 读取 FAT 文件存储的数据
 *
 * @param dst 数据保存位置
 * @param offset 偏移字节，必须是扇区大小的整数倍
 * @param size 读取字节数，必须是扇区大小的整数倍
 * @param entry 文件条目
 * @return 读取结果，0 成功，-1 失败
 */
static int fat_read(void *dst, off_t offset, size_t size, const fat_dir_entry *entry)
{
    assert(offset % SECT_SIZE == 0);
    assert(size % SECT_SIZE == 0);

    size_t read_bytes = 0;

    /**
     * 找到 offset 所在的簇号
     * 期间不断减小 offset 的值，将其转换为簇内偏移
     */
    const size_t clus_size = fat.bpb.sec_per_clus * SECT_SIZE;
    uint16_t cur_clus = entry->fst_clus;
    while (offset >= clus_size)
    {
        cur_clus = fat_next_clus(cur_clus);
        offset -= clus_size;
    }

    if (!fat_check_clus(cur_clus))
    {
        DEBUGK("warning: failed to find cluster number");
        return -1;
    }

    // 读取起始簇，要跳过簇内偏移的扇区
    lba_t lba = fat_clus2lba(cur_clus) + offset / SECT_SIZE;
    size_t read_size = MIN(clus_size - offset, size);
    if (0 > ata_read(dst + read_bytes, lba, read_size / SECT_SIZE))
    {
        return -1;
    }
    read_bytes += read_size;

    // 读取中间的完整簇
    while (size - read_bytes >= clus_size)
    {
        // 获取下一个簇号
        cur_clus = fat_next_clus(cur_clus);
        if (!fat_check_clus(cur_clus))
        {
            DEBUGK("warning: failed to find cluster number");
            return -1;
        }
        // 读取整个簇
        lba = fat_clus2lba(cur_clus);
        if (0 > ata_read(dst + read_bytes, lba, clus_size / SECT_SIZE))
        {
            DEBUGK("warning: failed to read disk");
            return -1;
        }
        read_bytes += clus_size;
    }

    // 读取结尾簇，仅读取前几个需要的扇区
    if (read_bytes < size)
    {
        cur_clus = fat_next_clus(cur_clus);
        if (!fat_check_clus(cur_clus))
        {
            DEBUGK("warning: failed to find cluster number");
            return -1;
        }
        lba = fat_clus2lba(cur_clus);
        read_size = size - read_bytes;
        if (0 > ata_read(dst + read_bytes, lba, read_size / SECT_SIZE))
        {
            DEBUGK("warning: failed to read disk");
            return -1;
        }
        read_bytes += read_size;
    }

    return 0;
}

/**
 * 读取文件
 *
 * @param dst 数据保存位置
 * @param offset 偏移字节
 * @param size 读取字节数
 * @param file 文件信息
 * @return 实际读取的字节数
 */
size_t file_read(void *dst, off_t offset, size_t size, file_struct *file)
{
    if (dst == NULL || file == NULL)
    {
        return 0;
    }

    // 不能超过文件上限
    if (offset >= file->fat_entry.file_size)
    {
        return 0;
    }
    size = MIN(size, file->fat_entry.file_size - offset);

    uint8_t buf[SECT_SIZE]; // 临时缓冲区
    size_t read_bytes = 0;  // 已读取字节数

    /**
     * 之前的 API 都需要以扇区为单位进行读写
     * 也就是说 offset 和 size 都必须是512的整数倍
     * 为了实现自由读写功能
     * 我们需要对开头和结尾的非扇区对齐部分进行特殊处理
     * 先以扇区对齐的方式读出，再保留扇区内需要的部分
     */

    // 起始地址未对齐扇区，先读取属于该扇区内的数据
    if (offset % SECT_SIZE != 0)
    {
        off_t sec_off = offset % SECT_SIZE;                // 扇区内偏移
        size_t read_size = MIN(size, SECT_SIZE - sec_off); // 该扇区内要读取的数据大小

        // 先读入临时缓冲区
        if (0 > fat_read(buf, offset - sec_off, SECT_SIZE, &file->fat_entry))
        {
            return read_bytes;
        }
        // 将需要的部分拷贝出来
        memcpy(dst, buf + sec_off, read_size);

        read_bytes += read_size;
    }

    // 读取中间包含的完整扇区
    if (size - read_bytes >= SECT_SIZE)
    {
        size_t read_size = ALIGN_DOWN(size - read_bytes, SECT_SIZE);

        if (0 > fat_read(dst + read_bytes, offset + read_bytes, read_size, &file->fat_entry))
        {
            return read_bytes;
        }

        read_bytes += read_size;
    }

    // 读取末尾非完整的扇区
    if (read_bytes < size)
    {
        size_t read_size = size - read_bytes;

        // 先读入临时缓冲区
        if (0 > fat_read(buf, offset + read_bytes, SECT_SIZE, &file->fat_entry))
        {
            return read_bytes;
        }
        // 将需要的部分拷贝出来
        memcpy(dst + read_bytes, buf, read_size);

        read_bytes += read_size;
    }

    return read_bytes;
}

void fs_init(void)
{
    static uint8_t buf[SECT_SIZE];

    // 遍历 MBR 分区表，找到首个引导分区作为文件系统所在分区
    const mbr_struct *mbr = (const mbr_struct *)buf;

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
     * 将 buffer 转换为 FAT 引导扇区结构体 fat_boot_sector 指针，以获取 BPB 和 EBPB
     * 然后验证文件系统类型是否为 FAT16
     */
    const fat_boot_sector *fbs = (const fat_boot_sector *)buf;
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
    fat.root_num_sectors = (fat.bpb.root_ent_cnt * sizeof(fat_dir_entry) + SECT_SIZE - 1) / SECT_SIZE; // 根目录占用扇区数量（向上取整）
    fat.data_start_lba = fat.root_start_lba + fat.root_num_sectors;                                  // 数据区起始扇区
}