/*
 * Copyright (C) 2025  Aleksa Radomirovic
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "wrap.h"

#include <errno.h>
#include <fcntl.h>

int inpipe[2], outpipe[2];

#if ! _GNU_SOURCE

static int pipe2_adjust_fd(int fd, unsigned int flags) {
    if(!flags) {
        return 0;
    }

    if(flags & ~(O_CLOEXEC | O_NONBLOCK)) {
        errno = ENOTSUP;
        return -1;
    }

    if(flags & O_NONBLOCK) {
        int fdfl = fcntl(fd, F_GETFL);
        if(fdfl == -1) {
            return -1;
        }

        fdfl |= O_NONBLOCK;
        if(fcntl(fd, F_SETFL, fdfl) == -1) {
            return -1;
        }
    }

    if(flags & O_CLOEXEC) {
        int fdfd = fcntl(fd, F_GETFD);
        if(fdfd == -1) {
            return -1;
        }

        fdfd |= FD_CLOEXEC;
        if(fcntl(fd, F_SETFD, fdfd) == -1) {
            return -1;
        }
    }

    return 0;
}

static int pipe2(int pipefd[2], unsigned int flags) {
    int ipipefd[2];
    if(pipe(ipipefd)) {
        return -1;
    }

    if(pipe2_adjust_fd(ipipefd[0], flags) || pipe2_adjust_fd(ipipefd[1], flags)) {
        return -1;
    }

    pipefd[0] = ipipefd[0];
    pipefd[1] = ipipefd[1];
    return 0;
}

#endif

int create_pipes() {
    if(pipe2(inpipe, O_CLOEXEC | O_NONBLOCK) || pipe2(outpipe, O_CLOEXEC | O_NONBLOCK)) {
        return errno;
    }

    return 0;
}

int direct_pipes() {
    if(dup2(inpipe[0], STDIN_FILENO) < 0) {
        return errno;
    }

    if(dup2(outpipe[1], STDOUT_FILENO) < 0) {
        return errno;
    }

    if(dup2(outpipe[1], STDERR_FILENO) < 0) {
        return errno;
    }

    return 0;
}

int cleanup_parent_pipes() {
    if(close(inpipe[1]) || close(outpipe[0])) {
        return errno;
    }

    return 0;
}

int cleanup_child_pipes() {
    if(close(inpipe[0]) || close(outpipe[1])) {
        return errno;
    }

    return 0;
}

int cleanup_pipes() {
    int status;

    status = cleanup_parent_pipes();
    if(status) {
        return status;
    }

    status = cleanup_child_pipes();
    if(status) {
        return status;
    }

    return 0;
}
