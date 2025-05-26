# FiRSTSTEP

面向教学的 x86 架构微型操作系统

## 参考资料

- [StevenBaby/onix - 操作系统实现](https://github.com/StevenBaby/onix)
- [6.828 / Fall 2018](https://pdos.csail.mit.edu/6.828/2018/schedule.html)
- [《Linux内核完全注释》 - 赵炯](https://download.oldlinux.org/CLK-5.0-WithCover.pdf)
- 《Orange'S：一个操作系统的实现》 - 于渊

## 开发环境

本项目需要 Linux 操作系统、GCC 编译器，以及 Bochs 和 QEMU 模拟器。

以下是本项目当前使用的开发环境（2025.5.26）：

| 项目     | 配置                                                     |
| -------- | -------------------------------------------------------- |
| 系统     | Windows Subsystem for Linux - Arch (2.4.13)              |
| 内核版本 | Linux 5.15.167.4-microsoft-standard-WSL2                 |
| 编译器   | gcc version 15.1.1 20250425 (GCC)                        |
| 模拟器   | Bochs x86 Emulator 2.8                                   |
| 模拟器   | QEMU emulator version 10.0.50 (v10.0.0-1165-g3c5a5e213e) |

### 虚拟机安装

由于本人使用的开发环境为基于 WSL2 的 Arch Linux，以下安装说明将使用 Arch 的包管理器进行说明。

Bochs 需要安装 AUR 中的 `bochs-sdl2` 和 `bochs-gdb-stub` 包：

- `bochs-sdl2`：支持图形界面的版本
- `bochs-gdb-stub`：支持连接 GDB 的版本，便于在 VSCode 中调试

```shell
yay -Sy bochs-sdl2 bochs-gdb-stub
```

**QEMU 并非必需**，但常用于快速运行验证，以及与 Bochs 的运行效果对比。

如果你使用的是本地安装或虚拟机中的 Linux，可通过以下命令安装：

```shell
sudo pacman -Sy qemu-system-x86
```

若使用 WSL2，为解决 QEMU 窗口无法通过 WSLg 显示的问题，推荐安装 AUR 中的 `qemu-arch-extra-git` 包：

```shell
yay -Sy qemu-arch-extra-git
```

## 项目运行

本项目通过 `make` 命令运行，所有构建目标可在项目根目录的 `Makefile` 中查看：

- `make all`：构建项目，包括程序编译、镜像创建和安装
- `make clean`：清除构建产物
- `make mount`：挂载镜像文件的首个分区至项目根目录下的 `mnt` 文件夹
- `make umount`：取消镜像挂载
- `make qemu`：构建并通过 QEMU 启动项目
- `make bochs`：构建并通过 Bochs 启动项目
- `make bochs-gdb`：构建并以 GDB 模式启动 Bochs

VSCode 中调试方式：在终端运行 `make bochs-gdb`，然后在 VSCode 中按 `F5` 启动调试。

## 目录结构

```
FiRSTSTEP/
├── bochsrc.cfg              # Bochs 配置文件
├── bochsrc.gdb              # Bochs GDB 配置文件
├── boot/                    # 引导程序（Stage1 和 Stage2）
│   ├── bootsect.S           # Stage1 程序，位于主引导记录（MBR）
│   ├── setup.S              # Stage2 程序入口
│   ├── setupmain.c          # Stage2 主逻辑
│   ├── link.ld              # Stage2 链接脚本
│   └── ...                  # 程序安装脚本与 Makefile 等
├── inc/                     # 头文件（公共接口）
│   ├── boot/                # 引导相关头文件
│   ├── kernel/              # 内核相关头文件
│   └── ...                  # 用户程序头文件与通用头文件（如 stdio.h, string.h 等）
├── kernel/                  # 内核源码
│   ├── entry.S              # 内核入口
│   ├── init.c               # 内核模块初始化流程
│   ├── gdt.c, idt.c, ...    # 各内核模块
│   └── Makefile             # 内核构建脚本
├── lib/                     # 通用库函数和用户程序库函数
├── usr/                     # 用户程序
│   ├── crt0.S               # 用户程序启动入口
│   ├── hello.c, init.c      # 示例用户程序
│   └── Makefile             # 用户程序构建脚本
├── mnt/                     # 挂载镜像的目录（运行时生成）
├── obj/                     # 编译产物文件（运行时生成）
├── Makefile                 # 项目总 Makefile
├── install_to_image.sh      # 将程序写入镜像的脚本
└── LICENSE                  # 项目许可证
```
