#pragma once

#include "fat16.h"

typedef struct file_struct
{
    fat_dir_entry fat_entry;
} file_struct;

int file_open(const char *path, file_struct *out_file);
size_t file_read(void *dst, off_t offset, size_t size, file_struct *file);