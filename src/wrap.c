/*
 * Copyright (C) 2025  Aleksa Radomirovic
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "wrap.h"

#include <argp.h>

char **program_args;

static const struct argp_option args_options[] = {
    { 0 }
};

static int args_parser(int key, char *arg, struct argp_state *state) {
    int status;

    switch(key) {
        case ARGP_KEY_NO_ARGS:
            argp_error(state, "no program specified");
            return EINVAL;

        case ARGP_KEY_ARGS:
            program_args = state->argv + state->next;
            status = run_program();
            if(status) {
                argp_failure(state, program_exit_status ? program_exit_status : 1, status, "failed to run program '%s'", *program_args);
            }
            return status;

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
    return argp_parse(&args_info, argc, argv, ARGP_IN_ORDER, NULL, NULL);
}
