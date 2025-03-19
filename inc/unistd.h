#pragma once

int syscall(int syscall_no, ...);
int write(int fd, const void *buf, int count);
int fork(void);