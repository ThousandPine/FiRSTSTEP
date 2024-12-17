SECTIONS
{
    . = 0x1000; /* 加载地址为 0x1000 */

    /* 将 .text 段放在开头 */
    .text : {
        *(.text)
    }

    /* 将 .data 段紧随 .text 段 */
    .data : {
        *(.data)
    }

    /* 将 .bss 段紧随 .data 段，确保空间分配 */
    .bss : {
        *(.bss)
    }
}
