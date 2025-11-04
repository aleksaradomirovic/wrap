/*
 * Copyright (C) 2025  Aleksa Radomirovic
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "wrap.h"

#include <fcntl.h>
#include <pty.h>
#include <unistd.h>

#define failure_setup_pty() failure_syscall("failed to set up pseudoterminal")
#define failure_close_pty() failure_syscall("failed to close pseudoterminal")

int master_read = -1, master_write = -1, slave_read = -1, slave_write = -1;

void setup_pty() {
    if(!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
        failure(EX_USAGE, 0, "both standard input and output must be a terminal or pseudoterminal!");
    }

    master_read = posix_openpt(O_RDWR | O_NOCTTY);
    if(master_read < 0) {
        failure_setup_pty();
    }

    master_write = master_read;

    const char *slave_path = ptsname(master_read);
    if(!slave_path) {
        failure_setup_pty();
    }

    if(unlockpt(master_read)) {
        failure_setup_pty();
    }

    slave_read = open(slave_path, O_RDWR);
    if(slave_read < 0) {
        failure_setup_pty();
    }

    slave_write = slave_read;
}

void assume_pty() {
    if(setsid() < 0) {
        failure_setup_pty();
    }

    if(dup2(slave_read, STDIN_FILENO) < 0 || dup2(slave_write, STDOUT_FILENO) < 0) {
        failure_setup_pty();
    }

    if(isatty(STDERR_FILENO)) {
        if(dup2(slave_write, STDERR_FILENO) < 0) {
            failure_setup_pty();
        }
    }

    close_pty_slave();
}

void close_pty_master() {
    if(master_write != master_read) {
        if(close(master_write)) {
            failure_close_pty();
        }
    }

    master_write = -1;

    if(close(master_read)) {
        failure_close_pty();
    }

    master_read = -1;
}

void close_pty_slave() {
    if(slave_write != slave_read) {
        if(close(slave_write)) {
            failure_close_pty();
        }
    }

    slave_write = -1;

    if(close(slave_read)) {
        failure_close_pty();
    }

    slave_read = -1;
}
