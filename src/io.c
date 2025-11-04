/*
 * Copyright (C) 2025  Aleksa Radomirovic
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "wrap.h"

#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <unistd.h>

#include <readline/history.h>
#include <readline/readline.h>

static void kill_wrap() {
    rl_callback_handler_remove();
    kill(wrap_pid, SIGTERM);
}

#define failure_loop() { int err = errno; kill_wrap(); errno = err; failure_io(); }

static void write_full(int fd, const void *buf, size_t length) {
    size_t total = 0;
    while(total < length) {
        ssize_t w = write(STDOUT_FILENO, buf + total, length - total);
        if(w < 0) {
            failure_loop();
        }

        total += w;
    }
}

static void ttysplice() {
    while(true) {
        unsigned char buf[0x4000];
        ssize_t length = read(master_read, buf, sizeof(buf));
        if(length < 0) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
#if defined(__linux__)
            if(errno == EIO) {
                break;
            }
#endif
            failure_loop();
        } else if(length == 0) {
            break;
        }

        write_full(STDOUT_FILENO, buf, length);
    }

    rl_on_new_line();
    rl_redisplay();
}

static void make_nonblocking(int fd) {
    int flags = fcntl(master_read, F_GETFL);
    if(flags == -1) {
        failure_loop();
    }

    flags |= O_NONBLOCK;
    if(fcntl(master_read, F_SETFL, flags) == -1) {
        failure_loop();
    }
}

static void readline_cb(char *line) {
    add_history_time(line);
    size_t len = strlen(line);
    line[len++] = '\n';
    write_full(master_write, line, len);
}

void loop_wrap() {
    make_nonblocking(STDIN_FILENO);
    make_nonblocking(master_read);

    rl_callback_handler_install("", readline_cb);
    using_history();

    struct pollfd polls[2] = {
        [0] = {
            .fd = STDIN_FILENO,
            .events = POLLIN,
        },

        [1] = {
            .fd = master_read,
            .events = POLLIN | POLLHUP,
        }
    };

    while(true) {
        int pollct = poll(polls, 2, -1);
        if(pollct < 0) {
            if(errno == EINTR) {
                continue;
            }

            failure_loop();
        } else if(pollct == 0) {
            continue;
        }

        if(polls[0].revents & POLLIN) {
            rl_callback_read_char();
        }

        if(polls[1].revents & POLLIN) {
            ttysplice();
        }

        if(polls[1].revents & POLLHUP) {
            break;
        }
    }

    rl_callback_handler_remove();
}
