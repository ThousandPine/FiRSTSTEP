# 标准启动文件
# 负责 main 函数执行之前的资源初始化，
# 以及 main 函数执行完毕后的资源回收

.global _start
.extern main, exit

.section .text
.code32
# 用户程序入口
_start:
    call    main
    
    # 调用 exit 退出程序
    push    %eax    # 使用 main 函数返回值作为参数
    call    exit
    hlt
