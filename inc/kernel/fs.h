#pragma once

#include "fat16.h"

typedef struct File
{
    FatDirEntry fat_entry;
} File;

void fs_init(void);
int file_open(const char *path, File *out_file);