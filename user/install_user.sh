#!/bin/bash

# 检查输入参数
if [ $# -ne 2 ]; then
    echo "Usage: $0 <file> <disk_image>"
    exit 1
fi

FILE="$1"
IMAGE_FILE="$2"

# 检查程序文件是否存在
if [ ! -f "$FILE" ]; then
    echo "Error: user file $FILE does not exist!"
    exit 1
fi

# 检查镜像文件是否存在
if [ ! -f "$IMAGE_FILE" ]; then
    echo "Error: Image file $IMAGE_FILE does not exist!"
    exit 1
fi

# 读取分区信息
PART_INFO=$(fdisk -l "$IMAGE_FILE")

if [ -z "$PART_INFO" ]; then
    echo "Error: Failed to read partition information!"
    exit 1
fi

# 查找带有引导分区标志 (*) 且文件系统为 FAT16 的分区
BOOT_PART=$(echo "$PART_INFO" | grep -E "\*.+FAT16")

if [ -z "$BOOT_PART" ]; then
    echo "Error: No FAT16 partition with a boot flag found!"
    exit 1
fi

# 提取分区起始扇区
START_SECTOR=$(echo "$BOOT_PART" | awk '{print $3}')
SECTOR_SIZE=$(fdisk -l "$IMAGE_FILE" | grep "Sector size" | awk '{print $4}')

# 计算挂载偏移量
OFFSET=$((START_SECTOR * SECTOR_SIZE))
echo "Boot partition found. Start sector: $START_SECTOR, Offset: $OFFSET"

# 创建临时目录
MOUNT_DIR=$(mktemp -d)

# 挂载引导分区
echo "Mounting the boot partition..."
sudo mount -o loop,offset="$OFFSET" "$IMAGE_FILE" "$MOUNT_DIR"

if [ $? -ne 0 ]; then
    echo "Error: Failed to mount the partition!"
    exit 1
fi

# 拷贝程序文件到分区根目录
echo "Copying the user file to the root directory of the partition..."
sudo cp "$FILE" "$MOUNT_DIR/"

if [ $? -ne 0 ]; then
    echo "Error: Failed to copy the user file!"
    sudo umount "$MOUNT_DIR"
    exit 1
fi

# 卸载分区
echo "Unmounting the partition..."
sudo umount "$MOUNT_DIR"

# 清理临时目录
rmdir "$MOUNT_DIR"

# 输出完成信息
echo "user file installation completed!"
