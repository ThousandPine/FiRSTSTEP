#pragma once

#include "types.h"

/**
 * FAT16 BIOS Parameter Block
 */
typedef struct bpb_struct
{
    uint16_t byte_per_sec;
    uint8_t sec_per_clus;
    uint16_t rsvd_sec_cnt;
    uint8_t num_fats;
    uint16_t root_ent_cnt;
    uint16_t tot_sec_16;
    uint8_t media;
    uint16_t sec_per_fat_16;
    uint16_t sec_per_track;
    uint16_t num_heads;
    uint32_t hidd_sec;
    uint32_t tot_sec_32;
} __attribute__((packed)) bpb_struct;

/**
 * FAT16 Extended BIOS Parameter Block
 */
typedef struct ebpb_struct
{
    uint8_t drv_num;
    uint8_t reserved_1;
    uint8_t boot_sig;
    uint32_t vol_id;
    uint8_t vol_lab[11];
    uint8_t fs_type[8];
} __attribute__((packed)) ebpb_struct;

/**
 * FAT16 分区的首个扇区
 */
typedef struct fat_boot_sector
{
    uint8_t jump_ins[3];
    uint8_t OEM[8];
    bpb_struct bpb;
    ebpb_struct ebpb;
} __attribute__((packed)) fat_boot_sector;

/**
 * FAT16 Directory Entry
 */
typedef struct fat_dir_entry
{
    uint8_t name[8];
    uint8_t ext[3];
    uint8_t attr;
    uint8_t NT_res;
    uint8_t crt_time_tenth;
    uint16_t crt_time;
    uint16_t crt_date;
    uint16_t lst_acc_date;
    uint16_t ea_index; // FAT12和FAT16中的EA-Index（OS/2和NT使用），FAT32中第一个簇的两个高字节
    uint16_t wrt_time;
    uint16_t wrt_date;
    uint16_t fst_clus;
    uint32_t file_size;
} __attribute__((packed)) fat_dir_entry;

// FAT Directory Entry File Attributes
#define FAT_ATTR_READ_ONLY 0x01 // 只读
#define FAT_ATTR_HIDDEN 0x02    // 隐藏
#define FAT_ATTR_SYSTEM 0x04    // 系统
#define FAT_ATTR_VOLUME_ID 0x08 // 卷标
#define FAT_ATTR_DIRECTORY 0x10 // 子目录
#define FAT_ATTR_ARCHIVE 0x20   // 档案
#define FAT_ATTR_DEVICE 0x40    // 设备（内部使用）
#define FAT_ATTR_UNUSED 0x80    // 未使用