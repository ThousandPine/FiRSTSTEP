OBJDIR := ./obj
TARGET := kernel

LD := ld
CC := gcc

CFLAGS := -m32                         # 生成 32 位代码
CFLAGS += -march=i386                  # 生成针对 i386 架构的代码（32 位 x86 架构），相当于禁用高级指令集
CFLAGS += -nostdlib                    # 禁止链接标准库
CFLAGS += -fno-builtin                 # 禁用内置函数，确保所有函数都从源代码中实现，而非从库中调用
CFLAGS += -I ../inc                    # 指定包含头文件的路径，查找 ../inc 目录下的头文件
CFLAGS += -MMD                         # 生成依赖文件 (.d 文件)，以便跟踪头文件变化
CFLAGS += -MP                          # 生成额外的依赖项目标，以防止删除的头文件导致依赖错误
CFLAGS += -Wall                        # 启用所有常见的警告信息
CFLAGS += -nostartfiles                # 禁用默认启动文件（如 crt0.o）
CFLAGS += -fno-stack-protector         # 禁用栈保护（防止栈溢出保护）
CFLAGS += -fno-pie                     # 禁用位置无关执行（Position-Independent Executable）
CFLAGS += -fno-pic                     # 禁用位置无关代码（Position-Independent Code）
# 上面的注释对齐带上了很多空格，要用 strip 去除多余空格
CFLAGS := $(strip ${CFLAGS})

# 指定生成 32 位 ELF 格式的可执行文件，适用于 i386 架构
LDFLAGS := -m elf_i386

SRC = $(wildcard *.c)                # 当前目录下所有的 .c 源文件
OBJ = $(SRC:%.c=$(OBJDIR)/%.o)		 # 将 SRC 中的 .c 文件扩展名替换为 .o，生成目标文件列表 OBJ
DEPS = $(SRC:%.c=$(OBJDIR)/%.d)      # 为 .c 文件生成依赖文件 .d，依赖文件存放在 $(OBJDIR) 目录下

all: $(TARGET)

$(OBJDIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

kernel: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^
	./install_kernel.sh $@ $(IMG_PATH)

# 引用生成的 .d 依赖文件，使其能够判断头文件依赖
-include $(DEPS)

clean:
	rm -rf $(TARGET) $(OBJDIR)
