/*
 * Copyright (C) 2025  Aleksa Radomirovic
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "wrap.h"

struct argp_state *prog_state;

char **wrap_args;

static const struct argp_option args_options[] = {
    { 0 }
};

static int args_parser(int key, char *arg, struct argp_state *restrict state) {
    prog_state = state;

    switch(key) {
        case ARGP_KEY_ARGS:
            wrap_args = state->argv + state->next;
            return 0;

        case ARGP_KEY_NO_ARGS:
            argp_error(prog_state, "no program specified");
            return EINVAL;

        case ARGP_KEY_SUCCESS:
            setup_pty();

            fork_wrap();
            close_pty_slave();
            loop_wrap();
            join_wrap();
            return 0;

        case ARGP_KEY_FINI:
            close_pty_master();
            return 0;

        default:
            return ARGP_ERR_UNKNOWN;
    }
}

static const struct argp args_info = {
    .options = args_options,
    .parser = args_parser,

    .args_doc = "PROGRAM [ARGS...]",
};

int main(int argc, char **argv) {
    int status = argp_parse(&args_info, argc, argv, ARGP_IN_ORDER, NULL, NULL);
    if(status) {
        return status;
    }
    return wrap_exit;
}
