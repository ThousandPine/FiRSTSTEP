OBJDIR := ../obj/lib
TARGET := libmylib.a

CC := gcc
AR := ar

# 外部调用时会定义默认的 CFLAGS，需要使用 override 关键字才能对其进行修改
# 否则 CFLAGS 会始终等于外部传入的初始值
override CFLAGS += -m32                         # 生成 32 位代码
override CFLAGS += -march=i386                  # 生成针对 i386 架构的代码（32 位 x86 架构），相当于禁用高级指令集
override CFLAGS += -nostdlib                    # 禁止链接标准库
override CFLAGS += -fno-builtin                 # 禁用内置函数，确保所有函数都从源代码中实现，而非从库中调用
override CFLAGS += -I ../inc                    # 指定包含头文件的路径，查找 ../inc 目录下的头文件
override CFLAGS += -MMD                         # 生成依赖文件 (.d 文件)，以便跟踪头文件变化
override CFLAGS += -MP                          # 生成额外的依赖项目标，以防止删除的头文件导致依赖错误
override CFLAGS += -Wall                        # 启用所有常见的警告信息
override CFLAGS += -nostartfiles                # 禁用默认启动文件（如 crt0.o）
override CFLAGS += -fno-stack-protector         # 禁用栈保护（防止栈溢出保护）
override CFLAGS += -fno-pie                     # 禁用位置无关执行（Position-Independent Executable）
override CFLAGS += -fno-pic                     # 禁用位置无关代码（Position-Independent Code）
# 上面的注释对齐带上了很多空格，要用 strip 去除多余空格
override CFLAGS := $(strip ${CFLAGS})

SRC = $(wildcard *.c)                # 当前目录下所有的 .c 源文件
OBJ = $(SRC:%.c=$(OBJDIR)/%.o)		 # 将 SRC 中的 .c 文件扩展名替换为 .o，生成目标文件列表 OBJ
DEPS = $(SRC:%.c=$(OBJDIR)/%.d)      # 为 .c 文件生成依赖文件 .d，依赖文件存放在 $(OBJDIR) 目录下

all: $(TARGET)

$(OBJDIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

$(TARGET): $(OBJ)
	$(AR) rcs $@ $^

# 引用生成的 .d 依赖文件，使其能够判断头文件依赖
-include $(DEPS)

clean:
	rm -rf $(TARGET) $(OBJDIR)
