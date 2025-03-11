/* FIXME: 使用 PHDRS 设置段的 RWX 属性 */
SECTIONS
{
    . = 0x40000000; /* 加载地址为 1 GiB */

    /* 将 .text 段放在开头 */
    .text : {
        *(.text)
    }

    .data : {
        *(.data)
    }

    .rodata : {
        *(.rodata)
    }

    .bss : {
        *(.bss)
    }
}
