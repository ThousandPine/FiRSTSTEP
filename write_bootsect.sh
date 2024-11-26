#!/bin/bash

# 参数检查
if [ $# -ne 2 ]; then
    echo "Usage: $0 <bin_file> <disk_image>"
    exit 1
fi

BIN_FILE=$1          # 第一个参数是引导程序二进制文件
DISK_IMAGE=$2        # 第二个参数是磁盘镜像文件

# 检查文件是否存在
if [ ! -f "$BIN_FILE" ]; then
    echo "Error: Binary file '$BIN_FILE' not found."
    exit 1
fi

if [ ! -f "$DISK_IMAGE" ]; then
    echo "Error: Disk image '$DISK_IMAGE' not found."
    exit 1
fi

# 检查BIN_FILE是否小于等于446字节
BIN_SIZE=$(stat -c%s "$BIN_FILE")
if [ $BIN_SIZE -gt 446 ]; then
    echo "Error: Binary file '$BIN_FILE' is larger than 446 bytes."
    exit 1
fi

# 写入引导代码（前 446 字节）
dd if="$BIN_FILE" of="$DISK_IMAGE" bs=1 count=446 conv=notrunc status=none

# 写入引导标志（最后两字节）
echo -ne '\x55\xAA' | dd of="$DISK_IMAGE" bs=1 seek=510 conv=notrunc status=none

echo "Bootsect code successfully written to MBR of '$DISK_IMAGE'."
