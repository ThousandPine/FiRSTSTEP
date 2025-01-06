#include "kernel/x86.h"
#include "kernel/kernel.h"
#include "kernel/ata.h"

/**
 * 等待 BSY 状态位更新
 *
 * 按照 ATA 标准
 * 在写入命令寄存器后的 400 ns 内状态寄存器中的 BSY 位设置为 1
 */
static void ata_bsy_delay()
{
    // 延迟 400ns 等待状态更新
    for (int i = 0; i < 4; i++)
    {
        inb(ATA_REG_STATUS);
    }
}

/**
 * 等待设备就绪
 *
 * “就绪”也可能表示出错，因此要进一步判断状态值
 *
 * @return 设备状态值，-1 表示等待超时
 */
static int ata_device_ready(void)
{
    ata_bsy_delay();

    /**
     * 轮询状态寄存器，等待设备就绪
     *
     * 等待 BSY 状态结束且 DRDY 置位
     * retries 表示最大轮询次数
     */
    for (int retries = 100000; retries--;)
    {
        uint8_t status = inb(ATA_REG_STATUS);
        if ((status & (ATA_SR_BSY | ATA_SR_DRDY)) == ATA_SR_DRDY)
        {
            return status;
        }
    }

    // 循环结束说明超时
    DEBUGK("ATA device ready timeout");
    return -1;
}

/**
 * 等待数据传输就绪
 *
 * @return 0 成功，-1 失败
 */
static int ata_data_ready(void)
{
    // 等待设备就绪
    int status;
    if ((status = ata_device_ready()) < 0)
    {
        return -1;
    }

    // DRQ 置位表示数据就绪
    if (status & ATA_SR_DRQ)
    {
        return 0;
    }
    // 出现错误
    if (status & (ATA_SR_DF | ATA_SR_ERR))
    {
        DEBUGK("ATA device status error %u", status);
        return -1;
    }
    // 未知状态，可能是未处理任何请求时的空闲状态
    DEBUGK("ATA unknow status %u", status);
    return -1;
}

/**
 * LBA 48 寻址读取多个扇区数据
 *
 * @param dst 目标内存地址
 * @param lba LBA 48 起始地址，以扇区为单位
 * @param count 扇区数量，0 表示 65536 个扇区
 */
int ata_read(void *dst, lba_t lba, uint16_t count)
{
    // 检查是否超出 LBA48 的范围
    assert(lba < 0xFFFFFFFFFFFFULL);

    // 等待设备就绪
    if (ata_device_ready() < 0)
    {
        return -1;
    }

    /**
     * 写入读取扇区数与 LBA 地址
     *
     * 通过向一个端口写入两次数据组合成 16 bit 参数，先发送高字节再发送低字节
     *
     * 尽量不要连续两次向同一个 IO 端口发送字节
     * 这样做比向不同 IO 端口执行两个 outb() 命令要慢得多
     */
    outb(ATA_REG_SECCOUNT, count >> 8);   // 扇区计数高 8 位
    outb(ATA_REG_LBA0, lba >> 24);        // LBA[24:31]
    outb(ATA_REG_LBA1, lba >> 32);        // LBA[32:39]
    outb(ATA_REG_LBA2, lba >> 40);        // LBA[40:47]
    outb(ATA_REG_SECCOUNT, count & 0xFF); // 扇区计数低 8 位
    outb(ATA_REG_LBA0, lba);              // LBA[0:7]
    outb(ATA_REG_LBA1, lba >> 8);         // LBA[8:15]
    outb(ATA_REG_LBA2, lba >> 16);        // LBA[16:23]

    /**
     * 设置硬盘驱动器属性
     * 端口低四位用于 LBA28 和 CHS 地址位的补充
     * 高四位分别表示
     * Bit 4: 从属位，0 主驱动器，1 从驱动器
     * Bit 5: 已过时且未使用，始终为 1
     * Bit 6: 寻址模式，0 CHS，1 LBA
     * Bit 7: 已过时且未使用，始终为 1
     */
    outb(ATA_REG_HDDEVSEL, 0xE0);

    /**
     * 发送读取命令
     * ATA_CMD_READ_PIO 是参数为 8 bit 的读取命令
     * ATA_CMD_READ_PIO_EXT 是参数为 16 bit 的读取命令
     * LBA 48 的参数是 16 bit
     */
    outb(ATA_REG_COMMAND, ATA_CMD_READ_PIO_EXT);

    // 循环读取每个扇区，使用 do-while 是为了让 count 为 0 时循环 65536 次
    do
    {
        // 每个扇区读取前等待数据就绪
        if (ata_data_ready() < 0)
        {
            return -1;
        }
        insl(ATA_REG_DATA, dst, SECT_SIZE / 4); // 将数据读取到当前缓冲区
        dst += SECT_SIZE;                       // 移动缓冲区指针
    } while (--count);

    return 0;
}

/**
 * LBA 48 寻址写入多个扇区数据
 *
 * @param src 源内存地址
 * @param lba LBA 48 起始地址，以扇区为单位
 * @param count 扇区数量，0 表示 65536 个扇区
 */
int ata_write(const void *src, lba_t lba, uint16_t count)
{
    // 检查是否超出 LBA48 的范围
    assert(lba < 0xFFFFFFFFFFFFULL);

    // 等待设备就绪
    if (ata_device_ready() < 0)
    {
        return -1;
    }

    // 设置写入扇区数与 LBA 地址，参考 ata_read
    outb(ATA_REG_SECCOUNT, count >> 8);   // 扇区计数高 8 位
    outb(ATA_REG_LBA0, lba >> 24);        // LBA[24:31]
    outb(ATA_REG_LBA1, lba >> 32);        // LBA[32:39]
    outb(ATA_REG_LBA2, lba >> 40);        // LBA[40:47]
    outb(ATA_REG_SECCOUNT, count & 0xFF); // 扇区计数低 8 位
    outb(ATA_REG_LBA0, lba);              // LBA[0:7]
    outb(ATA_REG_LBA1, lba >> 8);         // LBA[8:15]
    outb(ATA_REG_LBA2, lba >> 16);        // LBA[16:23]

    // 设置硬盘驱动器属性，参考 ata_read
    outb(ATA_REG_HDDEVSEL, 0xE0);

    // 发送写入命令
    outb(ATA_REG_COMMAND, ATA_CMD_WRITE_PIO_EXT);

    // 循环写入每个扇区，使用 do-while 是为了让 count 为 0 时循环 65536 次
    do
    {
        // 每个扇区写入前等待设备就绪
        if (ata_data_ready() < 0)
        {
            return -1;
        }
        outsl(ATA_REG_DATA, src, SECT_SIZE / 4); // 将数据从当前缓冲区写入磁盘
        src += SECT_SIZE;                        // 移动缓冲区指针
    } while (--count);

    return 0;
}
