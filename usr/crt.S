# 临时的“标准启动文件”

.global _start
.extern main


.section .text
.code32
# 用户程序入口
_start:
    call    main
    hlt
