/*
 * Copyright (C) 2025  Aleksa Radomirovic
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <unistd.h>

extern int inpipe[2], outpipe[2];

int create_pipes();
int direct_pipes();
int cleanup_parent_pipes();
int cleanup_child_pipes();
int cleanup_pipes();

int io_loop();

extern pid_t program_pid;
extern int program_exit_status;

int fork_program();
int join_program();


extern char **program_args;

int run_program();
