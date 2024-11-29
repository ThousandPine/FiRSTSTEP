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

# 获取第一个分区的起始扇区
START_SECTOR=$(fdisk -l "$DISK_IMAGE" | awk '/^'$DISK_IMAGE'/ {print $2; exit}')

# 计算 MBR gap 大小（字节）
MBR_GAP_SIZE=$(( (START_SECTOR - 1) * 512 ))

# 获取 BIN_FILE 的大小（字节）
BIN_SIZE=$(stat -c%s "$BIN_FILE")

# 判断 BIN_FILE 是否小于 MBR gap
if [ $BIN_SIZE -gt $MBR_GAP_SIZE ]; then
    echo "Error: Binary file '$BIN_FILE' is larger than MBR gap."
    echo "MBR gap size: $MBR_GAP_SIZE bytes"
    echo "Binary file size: $BIN_SIZE bytes"
    exit 1
fi

# 计算 BIN_FILE 占用的扇区数量，向上取整
SECTOR_COUNT=$(( (BIN_SIZE + 511) / 512 ))

# 检查是否在合法范围内（1 ~ 256）
if [ $SECTOR_COUNT -eq 0 ]; then
    echo "Error: Invalid sector count calculated as 0. Check '$BIN_FILE' size."
    exit 1
elif [ $SECTOR_COUNT -gt 64 ]; then
    echo "Error: '$BIN_FILE' occupies too many sectors ($SECTOR_COUNT). Maximum is 64."
    exit 1
fi

# 将扇区数量写入 BIN_FILE 的首个字节
printf "\\x$(printf '%02x' $SECTOR_COUNT)" | dd of="$BIN_FILE" bs=1 count=1 conv=notrunc status=none

# 将 BIN_FILE 写入 MBR gap 的开头
dd if="$BIN_FILE" of="$DISK_IMAGE" bs=1 seek=512 count=$BIN_SIZE conv=notrunc status=none

echo "'$BIN_FILE' successfully written to MBR gap of '$DISK_IMAGE'."
