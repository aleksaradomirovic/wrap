/*
 * Copyright (C) 2025  Aleksa Radomirovic
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#define _GNU_SOURCE 1

extern char **program_argv;
extern int program_status;

extern int inpipe[2], outpipe[2];

int setup_io();
int cleanup_io();

int adopt_io();
int io_loop();

int setup_input();
int cleanup_input();
int handle_input();

int run_program();
