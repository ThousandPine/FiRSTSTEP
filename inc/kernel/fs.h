#pragma once

#include "fat16.h"

typedef struct File
{
    FatDirEntry fat_entry;
} File;

int file_open(const char *path, File *out_file);
size_t file_read(void *dst, off_t offset, size_t size, File *file);