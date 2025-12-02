/*
 * Copyright (C) 2025  Aleksa Radomirovic
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "wrap.h"

#include <argp.h>

char **program_args;

static int args_parser(int key, char *arg, struct argp_state *__restrict__ state) {
    switch(key) {
        case ARGP_KEY_ARGS:
            program_args = state->argv + state->next;
            return 0;
        case ARGP_KEY_SUCCESS:
            fork_program();
            io_loop();
            join_program();
            return 0;

        case ARGP_KEY_NO_ARGS:
            argp_error(state, "no program provided");
            return EINVAL;

        default:
            return ARGP_ERR_UNKNOWN;
    }
}

static const struct argp args_info = {
    .parser = args_parser,
    .args_doc = "PROGRAM [ARGS...]"
};

int main(int argc, char **argv) {
    return argp_parse(&args_info, argc, argv, ARGP_IN_ORDER, 0, NULL);
}
