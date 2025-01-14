#include "types.h"
#include "string.h"
#include "kernel/x86.h"

/**
 * CRT Controller (CRTC) Registers
 *
 * CRTC 包括了多个寄存器，出于节约端口的目的
 * 访问某个寄存器时，需要先向索引寄存器端口（0x3D4）写入目标寄存器索引
 * 然后通过数据寄存器端口（0x3D5）读写寄存器数据
 *
 * ref: http://www.osdever.net/FreeVGA/vga/crtcreg.htm
 */
#define CRTC_ADDR_REG 0x3D4 // CRTC 索引寄存器
#define CRTC_DATA_REG 0x3D5 // CRTC 数据寄存器

// CRTC 寄存索引值
#define CRTC_START_ADDR_H 0xC // 显示内存起始位置 - 高位
#define CRTC_START_ADDR_L 0xD // 显示内存起始位置 - 低位
#define CRTC_CURSOR_H 0xE     // 光标位置 - 高位
#define CRTC_CURSOR_L 0xF     // 光标位置 - 低位

#define CGA_BASE_ADDR 0xC0000000 + 0xB8000            // 显卡 CGA 模式内存起始位置
#define CGA_MEM_SIZE (0xC0000 - 0xB8000) // 显卡 CGA 模式内存大小
#define WIDTH 80
#define HIGHT 25
#define BLANK 0x00 // 默认填充字符
#define ATTR 0x07  // 默认字符属性，要有前景色才能显示光标（黑色前景会导致光标变黑不可见）

// ASCII 控制字符
#define ASCII_NUL 0x00
#define ASCII_ESC 0x1B
#define ASCII_LF 0x0A  // \n 换行，一般会有回车效果
#define ASCII_CR 0x0D  // \r 回车
#define ASCII_BS 0x08  // \b 仅退格，不删除字符
#define ASCII_DEL 0x7F // 删除
#define ASCII_HT 0x09  // \t 水平制表符
#define ASCII_VT 0x0B  // \v 垂直制表符
#define ASCII_FF 0x0C  // \f 换页

/**
 * 描述 tty 信息的结构体
 *
 * 由于目前只需要单个 tty 因此定义为静态全局变量
 */
static struct
{
    uint32_t vmem_base; // 在显存中的基地址，必须是 2 的倍数
                        // 不是在内存中的地址，而是相对于显存映射区域的地址，也就是在显存内部的偏移
    uint32_t vmem_size; // 显存大小上限
    uint32_t screen;    // 显示内容起始地址，参考 set_screen 函数说明
    uint32_t cursor;    // 光标位置，参考 set_cursor 函数说明
    uint8_t attr;       // 字符属性
} tty;

/**
 * 通过修改 CRTC 起始地址寄存器（Start Address Register）设置显示内容
 *
 * “起始地址”是相对于显存的地址，也就是从 0x0 算起的，而不是映射地址的 0xB8000
 * 而且不需要额外跳过字符的属性位，表示的就是整个字符（包括 ASCII 和属性位）的序号
 *
 * 比如有 3 个字符位于内存 0xB8000, 0xB8002, 0xB8004 ，对应的寄存器值就是 0, 1, 2
 * 并不是 0, 2, 4 或 0xB8000, 0xB8002, 0xB8004
 */
static void set_screen(void)
{
    outb(CRTC_ADDR_REG, CRTC_START_ADDR_H);
    outb(CRTC_DATA_REG, (tty.screen >> 8) & 0xff);
    outb(CRTC_ADDR_REG, CRTC_START_ADDR_L);
    outb(CRTC_DATA_REG, (tty.screen) & 0xff);
}

/**
 * 修改 CRTC 光标位置寄存器（Cursor Location Register）
 *
 * 与“起始地址寄存器”性质相同，使用相对于显存的地址，不需要额外跳过字符属性位
 */
static void set_cursor(void)
{
    outb(CRTC_ADDR_REG, CRTC_CURSOR_H); // 光标高地址
    outb(CRTC_DATA_REG, ((tty.cursor) >> 8) & 0xff);
    outb(CRTC_ADDR_REG, CRTC_CURSOR_L); // 光标低地址
    outb(CRTC_DATA_REG, ((tty.cursor)) & 0xff);
}

// 用空白字符填充显存
static void reset_vmem(void)
{
    uint8_t *p = (uint8_t *)CGA_BASE_ADDR + tty.vmem_base;
    uint8_t *p_end = (uint8_t *)CGA_BASE_ADDR + tty.vmem_base + tty.vmem_size;

    while (p < p_end)
    {
        *p++ = BLANK;
        *p++ = ATTR;
    }
}

// 设置光标所在位置的字符内容
static inline void set_char(char c)
{
    uint8_t *p = (uint8_t *)(CGA_BASE_ADDR + (tty.cursor << 1));
    *p++ = c;
    *p = tty.attr;
}

/**
 * 向 tty 写入信息
 *
 * 严格来说 tty 应该以设备的形式存在，该函数则是以设备的接口的形式暴露
 * 但是目前还没有实现设备的功能，所以暂时直接对外暴露该函数
 *
 * TODO: 实现更多 ASCII 控制字符功能
 * FIXME: 处理达到显存上限的情况
 *
 * @return 返回实际写入数量
 */
int tty_write(const char *buf, size_t n)
{
    size_t i = 0;
    for (; i < n; i++)
    {
        char c = *buf++;
        switch (c)
        {
        case ASCII_NUL:
            break;
        case ASCII_LF:
            // 包括回车和换行两个动作
            tty.cursor += WIDTH;
            tty.cursor -= tty.cursor % WIDTH;
            break;
        case ASCII_CR:
            tty.cursor -= tty.cursor % WIDTH;
            break;
        case ASCII_BS:
            // 最多退格到行首
            tty.cursor -= !!(tty.cursor % WIDTH);
            break;
        case ASCII_DEL:
            tty.cursor--;
            set_char(BLANK);
            break;

        default:
            set_char(c);
            tty.cursor++;
            break;
        }
    }

    // 光标在屏幕外时滚屏
    while (tty.cursor >= tty.screen + WIDTH * HIGHT)
    {
        tty.screen += WIDTH;
    }
    while (tty.cursor < tty.screen)
    {
        tty.screen -= WIDTH;
    }

    set_cursor();
    set_screen();
    return i;
}

void tty_clear(void)
{
    tty.screen = tty.vmem_base >> 1;
    tty.cursor = tty.vmem_base >> 1;

    reset_vmem();
    set_screen();
    set_cursor();
}

void tty_init(void)
{
    tty.vmem_base = 0; // 必须是 2 的倍数
    tty.vmem_size = CGA_MEM_SIZE;
    tty.attr = 0x07; // 白色字符

    tty_clear();
}