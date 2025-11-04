/*
 * Copyright (C) 2025  Aleksa Radomirovic
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <argp.h>
#include <stdlib.h>
#include <sysexits.h>

extern struct argp_state *prog_state;

extern char **wrap_args;
extern int wrap_exit;

extern int master_read, master_write, slave_read, slave_write;
extern pid_t wrap_pid;

void setup_pty();
void assume_pty();

void close_pty_master();
void close_pty_slave();

void fork_wrap();
void loop_wrap();
void join_wrap();

#define failure(status, errnum, fmt, ...) { argp_failure(prog_state, status, errnum, fmt, ##__VA_ARGS__); exit(status); }
#define failure_syscall(fmt, ...) failure(EX_OSERR, errno, fmt, ##__VA_ARGS__)
#define failure_io() failure_syscall("i/o error")
