/*
 * Copyright (C) 2025  Aleksa Radomirovic
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "wrap.h"

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <readline/history.h>
#include <readline/readline.h>

static int write_full(int fd, void *buf, size_t length) {
    for(size_t total = 0; total < length;) {
        size_t towrite = length - total;
        if(towrite > SSIZE_MAX) {
            towrite = SSIZE_MAX;
        }

        ssize_t actwrite = write(fd, buf + total, towrite);
        if(actwrite < 0) {
            return errno;
        }

        total += (size_t) actwrite;
    }

    return 0;
}

static int readline_status = 0;

static int readline_fn(char *line) {
    add_history(line);

    size_t len = strlen(line);
    line[len++] = '\n';
    return write_full(inpipe[1], line, len);
}

static void readline_cb(char *line) {
    readline_status = readline_fn(line);
    free(line);
}

int handle_input() {
    rl_callback_read_char();
    return readline_status;
}

int setup_input() {
    rl_callback_handler_install("", readline_cb);
    using_history();
    return 0;
}

int cleanup_input() {
    rl_callback_handler_remove();
    return 0;
}
