/*
 * Copyright (C) 2025  Aleksa Radomirovic
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "wrap.h"

#include <argp.h>
#include <stdlib.h>

#define DEFAULT_ENV "/usr/bin/env"

static char *env_bin = NULL;

char **program_argv;

static int setup_program_argv(struct argp_state *restrict state) {
    int count = state->argc - state->next;
    program_argv = malloc(sizeof(char *) * (count + 2));
    if(!program_argv) {
        return errno;
    }

    program_argv[0] = env_bin;
    for(int i = 0; i < count; i++) {
        program_argv[i + 1] = state->argv[state->next + i];
    }
    program_argv[count + 1] = NULL;

    return 0;
}

#define OPTION_USE_ENV 'E'

static const struct argp_option args_options[] = {
    { "use-env", OPTION_USE_ENV, "ENV", OPTION_ARG_OPTIONAL, "Specifies an 'env' binary to execute the program. Defaults to " DEFAULT_ENV " if no argument is given." },
    { 0 }
};

static int args_parser(int key, char *arg, struct argp_state *restrict state) {
    int status;

    switch(key) {
        case OPTION_USE_ENV:
            if(env_bin) {
                argp_error(state, "env already specified");
            }
            env_bin = (arg ? arg : DEFAULT_ENV);
            return 0;

        case ARGP_KEY_NO_ARGS:
            argp_error(state, "no program specified");
            return EINVAL;
        case ARGP_KEY_ARGS:
            status = setup_io();
            if(status) {
                goto fail_run;
            }

            status = setup_program_argv(state);
            if(status) {
                goto fail_run;
            }

            status = run_program();
            if(status) {
                goto fail_run;
            }
            return program_status;

          fail_run:
            argp_failure(state, EXIT_FAILURE, status, "failed to run program");
            return status;

        case ARGP_KEY_SUCCESS:
            cleanup_io();
            free(program_argv);
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
    return argp_parse(&args_info, argc, argv, ARGP_IN_ORDER, NULL, NULL);
}
