/*
 * Copyright (C) 2025  Aleksa Radomirovic
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "wrap.h"

#include <sys/wait.h>
#include <unistd.h>

int wrap_exit;

pid_t wrap_pid;

static void exec_wrap() {
    close_pty_master();

    assume_pty();

    execvp(*wrap_args, wrap_args);
    failure_syscall("failed to exec '%s'", *wrap_args);
}

void fork_wrap() {
    wrap_pid = fork();
    if(wrap_pid < 0) {
        failure_syscall("failed to fork");
    }

    if(wrap_pid == 0) {
        exec_wrap();
    }
}

void join_wrap() {
    int status;
    if(waitpid(wrap_pid, &status, 0) < 0) {
        failure_syscall("failed to recover program exit");
    }

    if(WIFEXITED(status)) {
        wrap_exit = WEXITSTATUS(status);
    } else if(WIFSIGNALED(status)) {
        wrap_exit = 128 + WTERMSIG(status);
    }
}
