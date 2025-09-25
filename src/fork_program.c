/*
 * Copyright (C) 2025  Aleksa Radomirovic
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "wrap.h"

#include <errno.h>
#include <sys/wait.h>

pid_t program_pid;
int program_exit_status;

static int exec_program() {
    int status;

    status = direct_pipes();
    if(status) {
        return status;
    }

    execvp(*program_args, program_args);
    return errno;
}

int fork_program() {
    program_pid = fork();
    if(program_pid < 0) {
        return errno;
    }

    if(program_pid == 0) {
        return exec_program();
    }

    return 0;
}

int join_program() {
    while(true) {
        int state;
        if(waitpid(program_pid, &state, 0) < 0) {
            return errno;
        }

        if(WIFEXITED(state)) {
            program_exit_status = WEXITSTATUS(state);
            return 0;
        }

        if(WIFSIGNALED(state)) {
            program_exit_status = 128 + WTERMSIG(state);
            return 0;
        }
    }
}
