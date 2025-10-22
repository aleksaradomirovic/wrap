/*
 * Copyright (C) 2025  Aleksa Radomirovic
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "wrap.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <readline/readline.h>
#include <unistd.h>

int inpipe[2], outpipe[2];

int setup_io() {
    if(pipe2(inpipe, O_CLOEXEC) || pipe2(outpipe, O_CLOEXEC | O_NONBLOCK)) {
        return errno;
    }

    return 0;
}

int cleanup_io() {
    if(
        close(inpipe[1]) ||
        close(outpipe[0])
    ) {
        return errno;
    }

    return 0;
}

int adopt_io() {
    if(
        dup2(inpipe[0], STDIN_FILENO) < 0 ||
        dup2(outpipe[1], STDOUT_FILENO) < 0 ||
        dup2(outpipe[1], STDERR_FILENO) < 0
    ) {
        return errno;
    }

    return 0;
}

static int copy_output() {
    while(true) {
        ssize_t splicelen = splice(outpipe[0], NULL, STDOUT_FILENO, NULL, 0x10000, SPLICE_F_NONBLOCK);
        if(splicelen < 0) {
            if(errno == EINTR) {
                continue;
            }

            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            return errno;
        } else if(splicelen == 0) {
            break;
        }
    }

    return 0;
}

int io_loop() {
    int status;

    if(
        close(inpipe[0]) < 0 ||
        close(outpipe[1]) < 0
    ) {
        return errno;
    }

    struct pollfd polls[2] = {
        {
            .fd = outpipe[0],
            .events = POLLIN | POLLHUP,
        },
        {
            .fd = STDIN_FILENO,
            .events = POLLIN,
        },
    };

    while(true) {
        int pollct = poll(polls, 2, -1);
        if(pollct < 0) {
            if(errno == EINTR) {
                continue;
            }

            return errno;
        } else if(pollct == 0) {
            continue;
        }

        if(polls[0].revents & POLLIN) {
            rl_reset_line_state();
            status = copy_output();
            rl_redisplay();
            if(status) {
                return status;
            }
        }

        if(polls[0].revents & POLLHUP) {
            break;
        }

        if(polls[1].revents & POLLIN) {
            status = handle_input();
            if(status) {
                return status;
            }
        }
    }

    return 0;
}
