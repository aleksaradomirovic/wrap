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
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>

#define ENV_EXE "/usr/bin/env"

static struct event *sigchld_event;
static pid_t child_process;

static void kill_child_process() {
    if(child_process != -1) {
        kill(child_process, SIGTERM);
        child_process = -1;
    }
}

static void sigchld_callback(int sig, short flags, void *arg) {
    if(sig != SIGCHLD) return;

    while(1) {
        int status;
        pid_t process = waitpid(-1, &status, WNOHANG);
        if(process == 0 || (process == -1 && errno == ECHILD)) {
            break;
        } else if(process == -1) {
            error(1, errno, "child process signal handling error");
        } else if(process == child_process) {
            if(WIFEXITED(status) || WIFSIGNALED(status)) {
                child_process = -1;

                if(event_base_loopbreak(evbase) != 0) {
                    error(1, errno, "event handling error");
                }

                if(event_del(sigchld_event) != 0) {
                    error(1, errno, "event handling error");
                }

                event_free(sigchld_event);
                break;
            }
        }
    }
}

int fork_child_process(int argc, char **argv) {
    child_process = -1;
    if(atexit(kill_child_process) != 0) {
        return -1;
    }

    sigchld_event = event_new(evbase, SIGCHLD, EV_SIGNAL | EV_PERSIST, sigchld_callback, NULL);
    if(sigchld_event == NULL) {
        return -1;
    }

    if(event_add(sigchld_event, NULL) != 0) {
        return -1;
    }

    pid_t proc = fork();
    if(proc == -1) {
        error(1, errno, "failed to fork child process");
    } else if(proc == 0) {
        if(setup_fork_pipes() != 0) {
            error(1, errno, "failed to setup child pipes");
        }

        argv[-1] = ENV_EXE;
        execv(argv[-1], argv - 1);
        error(1, errno, "failed to fork-exec child process");
    }
    child_process = proc;

    return 0;
}
