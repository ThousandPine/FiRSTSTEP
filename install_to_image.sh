#!/bin/bash
# 将文件拷贝到磁盘镜像引导分区
# Usage: ./install_to_image.sh <disk_image> <source_path> [dest_path]

############################################
#              彩色输出函数                 #
############################################

# 定义颜色代码
COLOR_ERROR="\033[31m"    # 错误信息红色
COLOR_WARN="\033[33m"     # 警告信息黄色
COLOR_SUCCESS="\033[32m"  # 成功信息绿色
COLOR_INFO="\033[37m"     # 普通信息白色
COLOR_RESET="\033[0m"     # 重置颜色

# 带颜色输出封装函数
# 参数 1：颜色类型（error/warn/success/info）
# 参数 2：输出信息内容
color_echo() {
    local color_type="$1"   # 接收颜色类型参数
    local message="$2"      # 接收信息内容参数
    local output_stream=">/dev/stdout"  # 默认输出到标准输出

    # 根据类型选择颜色和输出流
    case "$color_type" in
        error)
            color="${COLOR_ERROR}"
            output_stream=">&2"  # 错误信息输出到标准错误
            ;;
        warn)
            color="${COLOR_WARN}"
            output_stream=">&2"  # 警告信息输出到标准错误
            ;;
        success)
            color="${COLOR_SUCCESS}"
            ;;
        info)
            color="${COLOR_INFO}"
            ;;
        *)
            color="${COLOR_RESET}"  # 默认无颜色
            ;;
    esac

    # 执行带颜色的输出
    eval "echo -e \"${color}${message}${COLOR_RESET}\" $output_stream"
}

############################################
#              核心功能函数                #
############################################

# 验证命令行参数有效性
# 返回值：0 表示有效，非 0 表示无效
validate_arguments() {
    # 检查参数数量是否在2~3个之间
    if [[ $# -lt 2 || $# -gt 3 ]]; then
        color_echo error "Error: Incorrect number of arguments"
        color_echo info "Usage: $0 <disk_image> <source_path> [dest_path]"
        color_echo info "Examples:"
        color_echo info "  $0 disk.img kernel.bin          # Copy to root"
        color_echo info "  $0 disk.img config.txt /conf    # Copy to /conf"
        return 1
    fi
}

# 验证源文件/目录是否存在
# 参数 1：源文件路径
validate_source() {
    local src="$1"
    if [[ ! -e "$src" ]]; then
        color_echo error "Error: Source path '$src' does not exist"
        return 1
    fi
}

# 验证磁盘镜像文件有效性
# 参数 1：磁盘镜像路径
validate_image() {
    local img="$1"
    # 检查文件是否存在
    if [[ ! -f "$img" ]]; then
        color_echo error "Error: Disk image '$img' not found"
        return 1
    fi

    # 检查是否为有效磁盘镜像
    if ! fdisk -l "$img" &> /dev/null; then
        color_echo error "Error: '$img' is not a valid disk image"
        return 1
    fi
}

# 挂载目标分区到临时目录
# 参数 1：磁盘镜像路径
# 参数 2：挂载点目录路径
mount_partition() {
    local img="$1"
    local mount_dir="$2"

    # 提取FAT16引导分区信息
    local boot_info=$(fdisk -l "$img" | grep -E "\*.+FAT16") # 筛选带有引导分区标志 (*) 且文件系统为 FAT16 的输出
    if [[ -z "$boot_info" ]]; then
        color_echo error "Error: No FAT16 boot partition found"
        return 1
    fi

    # 计算分区偏移量
    local sector_start=$(awk '{print $3}' <<< "$boot_info")                 # 获取起始扇区
    local sector_size=$(fdisk -l "$img" | awk '/Sector size/ {print $4}')   # 获取扇区大小
    local offset=$(( sector_start * sector_size ))                          # 计算字节偏移量

    color_echo info "Mounting partition: offset=${offset} bytes"
    # 执行挂载操作
    if ! sudo mount -o loop,offset="$offset" "$img" "$mount_dir"; then
        color_echo error "Error: Mount operation failed"
        return 1
    fi
}

# 复制文件到挂载目录
# 参数 1：源路径
# 参数 2：目标完整路径
copy_to_image() {
    local src="$1"
    local dest="$2"

    color_echo info "Copying: $src → $dest"

    if [[ -d "$src" ]]; then
        # 如果源是目录，则 dest 必须是目录
        sudo mkdir -p "$dest" || return 1
        if ! sudo cp -rv "$src/"* "$dest/"; then
            color_echo error "Error: Directory copy failed"
            return 1
        fi
    else
        # 如果源是文件，则 dest 可以是目录或文件路径
        if [[ "${dest: -1}" == "/" ]]; then
            # 以 / 结尾，视为目录
            sudo mkdir -p "$dest" || return 1
            if ! sudo cp -v "$src" "$dest"; then
                color_echo error "Error: File copy failed"
                return 1
            fi
        else
            # 视为完整文件路径
            sudo mkdir -p "$(dirname "$dest")" || return 1
            if ! sudo cp -v "$src" "$dest"; then
                color_echo error "Error: File copy failed"
                return 1
            fi
        fi
    fi

    sync  # 强制同步写入磁盘
}

############################################
#                 主流程                    #
############################################

main() {
    # 验证命令行参数
    validate_arguments "$@" || exit 1

    # 解析参数
    local disk_image="$1"    # 磁盘镜像路径
    local src_path="$2"      # 源文件/目录路径
    local dest_path="${3:-/}" # 目标路径（默认根目录）

    # 验证源文件有效性
    validate_source "$src_path" || exit 1

    # 验证磁盘镜像有效性
    validate_image "$disk_image" || exit 1

    # 创建临时挂载目录
    local mount_dir
    if ! mount_dir=$(mktemp -d); then
        color_echo error "Error: Failed to create temporary directory"
        exit 1
    fi

    # 挂载分区
    if ! mount_partition "$disk_image" "$mount_dir"; then
        sudo rm -rf "$mount_dir"  # 挂载失败时清理目录
        exit 1
    fi

    # 执行文件复制
    if ! copy_to_image "$src_path" "$mount_dir/$dest_path"; then
        sudo umount "$mount_dir"    # 复制失败时卸载
        sudo rm -rf "$mount_dir"    # 删除临时目录
        exit 1
    fi

    # 卸载分区
    if ! sudo umount "$mount_dir"; then
        color_echo warn "Warning: Failed to unmount partition"
    fi

    # 清理临时资源
    sudo rm -rf "$mount_dir"
    color_echo success "[Success] File ${src_path} deployed to ${dest_path}"
}

# 启动主程序
main "$@"