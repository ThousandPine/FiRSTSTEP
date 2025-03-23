# 临时的“标准启动文件”

.global _start
.extern main, exit


.section .text
.code32
# 用户程序入口
_start:
    call    main
    # 使用 main 函数返回值作为参数
    push    %eax
    # 调用 exit 退出程序
    call    exit
    hlt
