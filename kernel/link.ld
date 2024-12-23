SECTIONS
{
    . = 0x100000; /* 加载地址为 0x100000 */

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
