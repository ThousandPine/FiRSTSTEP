#pragma once

#include "types.h"

#define ELF_MAGIC 0x464C457FU // 字符串 "\x7FELF" 的等价小端编码整数

/**
 * ELF Header 32 bit version
 */
typedef struct elf_header
{
    uint32_t e_magic; // 必须等于 ELF_MAGIC
    uint8_t e_ident[12];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} __attribute__((packed)) elf_header;

/**
 * Program Header 32 bit version
 */
typedef struct program_header
{
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
} __attribute__((packed)) program_header;

// Program Header Type
#define PT_NULL 0x00000000    // Program header table entry unused
#define PT_LOAD 0x00000001    // Loadable segment
#define PT_DYNAMIC 0x00000002 // Dynamic linking information
#define PT_INTERP 0x00000003  // Interpreter information
#define PT_NOTE 0x00000004    // Auxiliary information
#define PT_SHLIB 0x00000005   // Reserved
#define PT_PHDR 0x00000006    // Segment containing program header table itself
#define PT_TLS 0x00000007     // Thread-Local Storage template

// Program Header Flags
#define PF_X 0x1 // Executable segment
#define PF_W 0x2 // Writeable segment
#define PF_R 0x4 // Readable segment