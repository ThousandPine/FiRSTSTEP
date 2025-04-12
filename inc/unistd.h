#pragma once

#include "types.h"
#include "waitflags.h"

int syscall(int syscall_no, ...);
int write(int fd, const void *buf, int count);
int fork(void);
pid_t getpid(void);
void exit(int status);
pid_t wait(int *status);
pid_t waitpid(pid_t pid, int *status, int options);
int execl(const char *path, const char *arg0, ...);