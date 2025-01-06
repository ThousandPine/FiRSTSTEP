#include "kernel/fs.h"
#include "kernel/ata.h"
#include "kernel/mbr.h"
#include "kernel/fat16.h"
#include "kernel/kernel.h"

static uint8_t buf[SECT_SIZE];
static PartitionEntry part = {0};
static struct
{
    uint32_t fat_start_lba;    // FAT 表起始扇区（LBA 格式）
    uint32_t root_start_lba;   // 根目录起始扇区（LBA 格式）
    uint32_t data_start_lba;   // 数据区起始扇区（LBA 格式）
    uint32_t root_num_sectors; // 根目录占用扇区数量（向上取整）
} fat;

void fs_init(void)
{
    // 遍历 MBR 分区表，找到首个引导分区作为文件系统所在分区
    const MBR *mbr = (const MBR *)buf;

    ata_read(mbr, 0, 1);
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
    ata_read(buf, part.start_lba, 1);
    BPB bpb = ((FatBootSector *)buf)->bpb;
    EBPB ebpb = ((FatBootSector *)buf)->ebpb;

    // 判断是否为 FAT16
    for (uint32_t i = 0; i < 5; i++)
    {
        if (ebpb.fs_type[i] != "FAT16"[i])
        {
            panic("Partition file system is not FAT16");
        }
    }

    // 计算 FAT16 各区域参数
    fat.fat_start_lba = part.start_lba + bpb.rsvd_sec_cnt;                                       // FAT 表起始扇区
    fat.root_start_lba = fat.fat_start_lba + (bpb.num_fats * bpb.sec_per_fat_16);                // 根目录起始扇区
    fat.root_num_sectors = (bpb.root_ent_cnt * sizeof(FatDirEntry) + SECT_SIZE - 1) / SECT_SIZE; // 根目录占用扇区数量（向上取整）
    fat.data_start_lba = fat.root_start_lba + fat.root_num_sectors;                              // 数据区起始扇区
}