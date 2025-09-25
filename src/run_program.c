/*
 * Copyright (C) 2025  Aleksa Radomirovic
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "wrap.h"

int run_program() {
    int status;

    status = create_pipes();
    if(status) {
        return status;
    }
    
    status = fork_program();
    if(status) {
        return status;
    }

    status = cleanup_child_pipes();
    if(status) {
        return status;
    }

    status = io_loop();
    if(status) {
        return status;
    }

    status = cleanup_parent_pipes();
    if(status) {
        return status;
    }

    status = join_program();
    if(status) {
        return status;
    }

    return 0;
}
