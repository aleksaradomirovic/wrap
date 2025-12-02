/*
 * Copyright (C) 2025  Aleksa Radomirovic
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "wrap.h"

#include <fcntl.h>
#include <poll.h>
#include <pty.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

#include <readline/readline.h>

pid_t program_pid = -1;
int master_fd;
int program_result;

void fork_program() {
    struct termios terminfo;
    if(tcgetattr(STDIN_FILENO, &terminfo)) {
        ERROR_GENERIC();
    }

    terminfo.c_lflag &= ~ECHO;

    program_pid = forkpty(&master_fd, NULL, &terminfo, NULL);
    if(program_pid < 0) {
        ERROR_GENERIC();
    }

    if(program_pid == 0) {
        execvp(*program_args, program_args);
        ERROR_GENERIC();
    }
}

static void make_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL);
    if(flags == -1) {
        ERROR_GENERIC();
    }
    flags |= O_NONBLOCK;
    if(fcntl(fd, F_SETFL, flags) == -1) {
        ERROR_GENERIC();
    }
}

static int dupnb(int fd) {
    fd = dup(fd);
    if(fd < 0) {
        ERROR_GENERIC();
    }
    make_nonblock(fd);
    return fd;
}

static void ckd_close(int fd) {
    if(close(fd)) {
        ERROR_GENERIC();
    }
}

static void copy_output(int pollmaster) {
    while(true) {
        unsigned char buf[0x10000];
        ssize_t rlen = read(pollmaster, buf, sizeof(buf));
        if(rlen < 0) {
            if(errno == EINTR) {
                continue;
            } else if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EIO) {
                break;
            }

            ERROR_GENERIC();
        } else if(rlen == 0) {
            break;
        }
        
        for(size_t total = 0; total < rlen;) {
            ssize_t wlen = write(STDOUT_FILENO, buf + total, rlen - total);
            if(wlen < 0) {
                if(errno == EINTR) {
                    continue;
                }

                ERROR_GENERIC();
            }

            total += wlen;
        }
    }
}

static void line_handler_cb(char *line) {
    size_t len = strlen(line);
    line[len++] = '\n';
    for(size_t total = 0; total < len;) {
        ssize_t wlen = write(master_fd, line + total, len - total);
        if(wlen < 0) {
            if(errno == EINTR) {
                continue;
            }

            ERROR_GENERIC();
        }

        total += wlen;
    }
    free(line);
}

void io_loop() {
    int pollstdin = dupnb(STDIN_FILENO), pollmaster = dupnb(master_fd);
    rl_callback_handler_install("", line_handler_cb);

    struct pollfd polls[2] = {
        [0] = {
            .fd = pollstdin,
            .events = POLLIN | POLLHUP,
        },
        [1] = {
            .fd = pollmaster,
            .events = POLLIN | POLLHUP,
        }
    };

    while(true) {
        int poll_count = poll(polls, 2, -1);
        if(poll_count < 0) {
            if(errno == EINTR) {
                continue;
            }

            ERROR_GENERIC();
        }

        if(polls[0].revents & POLLIN) {
            rl_callback_read_char();
            if(fflush(stdin)) {
                ERROR_GENERIC();
            }
        }

        if(polls[0].revents & POLLHUP) {
            ckd_close(STDIN_FILENO);
            break;
        }

        if(polls[1].revents & POLLIN) {
            copy_output(pollmaster);
        }

        if(polls[1].revents & POLLHUP) {
            ckd_close(STDOUT_FILENO);
            break;
        }
    }

    ckd_close(pollstdin);
    ckd_close(pollmaster);

    rl_callback_handler_remove();
}

void join_program() {
    int status;
    if(waitpid(program_pid, &status, 0) < 0) {
        ERROR_GENERIC();
    }

    if(WIFEXITED(status)) {
        program_result = WEXITSTATUS(status);
    } else if(WIFSIGNALED(status)) {
        program_result = 128 + WTERMSIG(status);
    } else {
        ERROR(0, "unknown exit status");
    }

    if(program_result) {
        exit(program_result);
    }
}
