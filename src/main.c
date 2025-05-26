/*
 * Copyright 2025 Aleksa Radomirovic
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "wrap.h"

#include <errno.h>
#include <error.h>
#include <event2/event.h>

struct event_base *evbase;

int main(int argc, char **argv) {
    if(argc < 2) {
        error(1, 0, "not enough arguments");
    }

    // event_enable_debug_logging(EVENT_DBG_ALL);

    evbase = event_base_new();
    if(evbase == NULL) {
        error(1, errno, "failed to allocate event base");
    }
    
    if(init_pipes() != 0) {
        error(1, errno, "failed to set up pipes");
    }

    if(fork_child_process(argc - 1, argv + 1) != 0) {
        error(1, errno, "failed to fork child process");
    }

    if(event_base_dispatch(evbase) != 0) {
        error(1, errno, "event loop error");
    }

    cleanup_pipes();
    event_base_free(evbase);
    return 0;
}
