#pragma once

#include "kernel/system.hpp"

int64_t handle_read(const regs* r);
int64_t handle_write(const regs* r);
int64_t handle_open(const regs* r);
int64_t handle_close(const regs* r);

int64_t handle_mmap(const regs* r);
int64_t handle_munmap(const regs* r);

int64_t handle_exec(const regs* r);
int64_t handle_fork(const regs* r);
int64_t handle_getpid(const regs* r);
int64_t handle_sleep(const regs* r);
int64_t handle_exit(const regs* r);
int64_t handle_set_fs_base(const regs* r);
int64_t handle_gettid(const regs* r);
int64_t handle_waitpid(const regs* r);
int64_t handle_kill(const regs* r);

int64_t handle_opendir(const regs* r);
int64_t handle_readdir(const regs* r);
int64_t handle_closedir(const regs* r);
int64_t handle_readentries(const regs* r);
int64_t handle_getstat(const regs* r);
int64_t handle_chdir(const regs* r);
int64_t handle_mkdir(const regs* r);
int64_t handle_getcwd(const regs* r);

int64_t handle_clock_get(const regs* r);
