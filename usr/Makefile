OBJDIR := ../obj/usr
LIBDIR := ../lib
LIBNAME := libmylib.a

LD := ld
CC := gcc

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


LDFLAGS := -m elf_i386 			# 指定生成 32 位 ELF 格式的可执行文件，适用于 i386 架构
LDFLAGS += -T link.ld  			# 使用 link.ld 链接脚本
LDFLAGS += -L $(LIBDIR)  		# 指定静态链接库所在目录为 ../lib
LDFLAGS += -l :$(LIBNAME)  		# 指定静态链接库名称，使用文件全名要加冒号 : ，格式参考 ld 的 -l 参数说明
LDFLAGS += -e _start			# 指定程序入口为 _start
# 上面的注释对齐带上了很多空格，要用 strip 去除多余空格
LDFLAGS := $(strip ${LDFLAGS})


CRT_SRC := $(wildcard crt*.S)									# 当前目录下所有的标准启动文件
CRT_OBJ := $(CRT_SRC:%.S=$(OBJDIR)/%.o) 						# 将 CRT_SRC 中的 .S 文件扩展名替换为 .o
SRC_C := $(wildcard *.c)                						# 当前目录下所有的 .c 源文件
OBJ_C := $(SRC_C:%.c=$(OBJDIR)/%.o)								# 将 SRC 中的 .c 文件扩展名替换为 .o
DEPS := $(CRT_OBJ:%.o=$(OBJDIR)/%.d) $(OBJ_C:%.o=$(OBJDIR)/%.d)	# 为 .o 文件生成依赖文件 .d

# 每个 .c 文件对应一个可执行文件，例如 foo.c -> ../obj/usr/foo
TARGETS := $(patsubst %.c,$(OBJDIR)/%,$(SRC_C))

all: $(TARGETS)

$(OBJDIR)/%.o: %.S
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

# 应用程序文件名和.o文件名都使用通配符的情况下
# Makefile 会将.o文件当作中间文件来处理
# 导致执行完成后.o文件被自动删除，如 rm ../obj/usr/init.o
$(OBJDIR)/%: $(OBJDIR)/%.o $(CRT_OBJ) $(LIBDIR)/$(LIBNAME)
# 链接用户程序
# 此处将 $(LDFLAGS) 后置是为了让 -l 指定的库文件出现在 $(OBJ) 文件之后（右侧），否则会编译出错
# 这与链接器从左到右依次解析符号的顺序有关，要让链接器先分析完 $(OBJ) 中缺失的符号，然后再从库文件链接需要的内容
	$(LD) -o $@ $< $(CRT_OBJ) $(LDFLAGS)
# 使用脚本安装程序到镜像文件引导分区的 /bin/ 目录下
	../install_to_image.sh $(IMG_PATH) $@ /bin/

# 引用生成的 .d 依赖文件，使其能够判断头文件依赖
-include $(DEPS)

# 伪目标，避免 make 删除中间文件
# 因为应用程序文件名和 .o 文件名都使用通配符的情况下
# make 会将 .o 文件当作中间文件来处理
# 导致执行完成后 .o 文件被自动删除，如 rm ../obj/usr/init.o
.SECONDARY:

clean:
	rm -rf $(TARGETS) $(OBJDIR)
