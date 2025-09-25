/*
 * Copyright (C) 2025  Aleksa Radomirovic
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "wrap.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>

#if ! _GNU_SOURCE

static ssize_t splice(int fd_in, off_t *off_in, int fd_out, off_t *off_out, size_t len, unsigned int flags) {
    if(flags || off_in || off_out) {
        errno = ENOTSUP;
        return -1;
    }

    char buf[len];
    ssize_t read_length = read(fd_in, buf, len);
    if(read_length < 0) {
        return -1;
    }

    size_t total = 0;
    while(total < (size_t) read_length) {
        ssize_t write_length = write(fd_out, buf + total, (size_t) read_length - total);
        if(write_length < 0) {
            return -1;
        }

        total += (size_t) write_length;
    }

    return read_length;
}

#endif

static int copy_out() {
    while(true) {
        ssize_t splice_length = splice(outpipe[0], NULL, STDOUT_FILENO, NULL, 0x2000, 0);
        if(splice_length < 0) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            return errno;
        }

        if(splice_length == 0) {
            break;
        }
    }

    return 0;
}

int io_loop() {
    struct pollfd polls[2] = {
        {
            .fd = STDIN_FILENO,
            .events = POLLIN,
        },
        {
            .fd = outpipe[0],
            .events = POLLIN | POLLHUP
        }
    };

    while(true) {
        int pollcnt = poll(polls, 2, -1);
        if(pollcnt < 0) {
            if(errno == EINTR) {
                continue;
            }

            return errno;
        }

        if(pollcnt == 0) {
            continue;
        }

        if(polls[1].revents & POLLIN) {
            int status = copy_out();
            if(status) {
                return status;
            }
        }

        if(polls[1].revents & POLLHUP) {
            break;
        }
    }

    return 0;
}
