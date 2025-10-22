/*
 * Copyright (C) 2025  Aleksa Radomirovic
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "wrap.h"

#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>

static pid_t program_pid;
int program_status;

static int exec_program() {
    int status;

    status = adopt_io();
    if(status) {
        return status;
    }

    if(program_argv[0] == NULL) {
        execvp(program_argv[1], program_argv + 1);
    } else {
        execv(program_argv[0], program_argv);
    }

    return errno;
}

static int fork_program() {
    program_pid = fork();
    if(program_pid < 0) {
        return errno;
    }

    if(program_pid == 0) {
        return exec_program();
    }

    return 0;
}

static int join_program() {
    int wstatus;

    if(waitpid(program_pid, &wstatus, 0) < 0) {
        return errno;
    }

    if(WIFEXITED(wstatus)) {
        program_status = WEXITSTATUS(wstatus);
    } else if(WIFSIGNALED(wstatus)) {
        program_status = 128 + WTERMSIG(wstatus);
    } else {
        return EINVAL;
    }

    return 0;
}

static int kill_program() {
    if(kill(program_pid, SIGTERM)) {
        return errno;
    }

    return 0;
}

int run_program() {
    int status;

    status = fork_program();
    if(status) {
        return status;
    }

    status = setup_input();
    if(status) {
        goto fail;
    }

    status = io_loop();
    if(status) {
        cleanup_input();
        goto fail;
    }

    status = cleanup_input();
    if(status) {
        return status;
    }

    status = join_program();
    if(status) {
        return status;
    }

    return 0;

  fail:
    kill_program();
    return status;
}
