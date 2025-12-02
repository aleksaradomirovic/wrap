/*
 * Copyright (C) 2025  Aleksa Radomirovic
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <errno.h>
#include <error.h>
#include <signal.h>
#include <stddef.h>
#include <sysexits.h>
#include <unistd.h>

#define ERROR(errnum, fmt, ...) { if(program_pid >= 0) { kill(program_pid, SIGTERM); }; error(EX_SOFTWARE, errnum, fmt, ##__VA_ARGS__); unreachable(); }
#define ERROR_GENERIC() ERROR(errno, "error");

extern char **program_args;
extern pid_t program_pid;
extern int master_fd;
extern int program_result;

void fork_program();
void io_loop();
void join_program();
