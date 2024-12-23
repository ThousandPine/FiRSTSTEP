#pragma once

#include "types.h"

/**
 * FAT16 BIOS Parameter Block
 */
struct BPB
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
} __attribute__((packed));

/**
 * FAT16 Extended BIOS Parameter Block
 */
struct EBPB
{
    uint8_t drv_num;
    uint8_t reserved_1;
    uint8_t boot_sig;
    uint32_t vol_id;
    uint8_t vol_lab[11];
    uint8_t fs_type[8];
} __attribute__((packed));

/**
 * FAT16 分区的首个扇区
 */
struct FatBootSector
{
    uint8_t jump_ins[3];
    uint8_t OEM[8];
    struct BPB bpb;
    struct EBPB ebpb;
} __attribute__((packed));

/**
 * FAT16 Directory Entry
 */
struct FatDirEntry
{
    uint8_t name[11];
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
} __attribute__((packed));