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

#include <event2/util.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <termios.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TTY_FILE "/dev/tty"

static int tty;
static struct termios tty_settings_old, tty_settings;

struct pipe {
    int fds[2];
    struct event *ev;
};

static struct pipe stdpipes[2];

static char *linebuf = NULL;
static size_t linelen = 0, linecap = 0;

#define SAVE_CURSOR "\e[s"
#define LOAD_CURSOR "\e[u"
#define RESET_LINE LOAD_CURSOR "\e[0J"

void cleanup_pipes() {
    if(tty != -1) {
        tcsetattr(tty, TCSANOW, &tty_settings_old);
        close(tty);
        tty = -1;
    }

    for(size_t i = 0; i < 2; i++) {
        for(size_t j = 0; j < 2; j++) {
            if(stdpipes[i].ev != NULL) {
                event_free(stdpipes[i].ev);
                stdpipes[i].ev = NULL;
            }
            if(stdpipes[i].fds[j] != -1) {
                close(stdpipes[i].fds[j]);
                stdpipes[i].fds[j] = -1;
            }
        }
    }

    if(linebuf != NULL) {
        free(linebuf);
        linebuf = NULL;
    }
}

static int init_pipe_fds() {
    tty = open(TTY_FILE, O_RDWR);
    if(tty == -1) {
        return -1;
    }

    if(evutil_make_socket_closeonexec(tty) != 0) {
        return -1;
    }

    for(size_t i = 0; i < 2; i++) {
        if(pipe(stdpipes[i].fds) != 0) {
            return -1;
        }
    }

    if(evutil_make_socket_nonblocking(STDIN_FILENO) != 0) {
        return -1;
    }

    if(evutil_make_socket_nonblocking(stdpipes[STDOUT_FILENO].fds[0]) != 0) {
        return -1;
    }

    if(evutil_make_socket_closeonexec(stdpipes[STDIN_FILENO].fds[1]) != 0) {
        return -1;
    }

    if(evutil_make_socket_closeonexec(stdpipes[STDOUT_FILENO].fds[0]) != 0) {
        return -1;
    }

    return 0;
}

static int writeall(int fd, const void *buf, size_t len) {
    while(len > 0) {
        ssize_t wlen = write(fd, buf, len);
        if(wlen < 0) {
            return -1;
        } else if(wlen == 0) {
            errno = EIO;
            return -1;
        }

        buf += wlen;
        len -= wlen;
    }

    return 0;
}

static void stdin_cb(int fd, short event, void *arg) {
    while(1) {
        char buf[4096];
        ssize_t len = read(fd, buf, sizeof(buf));
        if(len < 0) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            error(1, errno, "pipe error");
        } else if(len == 0) {
            break;
        }

        for(size_t i = 0; i < (size_t) len; i++) {
            char c = buf[i];

            if(linelen == linecap) {
                size_t ncap;
                if(__builtin_add_overflow(linecap, 4096, &ncap)) {
                    errno = EOVERFLOW;
                    error(1, errno, "input size overflow");
                }

                char *nline = realloc(linebuf, ncap);
                if(nline == NULL) {
                    error(1, errno, "input size overflow");
                }

                linebuf = nline;
                linecap = ncap;
            }

            if(c == '\n' || c == tty_settings.c_cc[VEOL]) {
                linebuf[linelen++] = '\n';
                if(writeall(stdpipes[STDIN_FILENO].fds[1], linebuf, linelen) != 0) {
                    error(1, errno, "pipe error");
                }

                linelen = 0;

                if(writeall(tty, "\n" SAVE_CURSOR, strlen("\n" SAVE_CURSOR)) != 0) {
                    error(1, errno, "pipe error");
                }

                continue;
            }

            if(c == tty_settings.c_cc[VERASE]) {
                if(linelen > 0) {
                    if(writeall(tty, "\b \b", strlen("\b \b")) != 0) {
                        error(1, errno, "pipe error");
                    }
                    linelen--;
                }

                continue;
            }

            if(c < ' ') {
                continue;
            }

            if(writeall(tty, &c, 1) != 0) {
                error(1, errno, "pipe error");
            }

            linebuf[linelen++] = c;
        }
    }
}

static void stdout_cb(int fd, short event, void *arg) {
    if(writeall(tty, RESET_LINE, strlen(RESET_LINE)) != 0) {
        error(1, errno, "pipe error");
    }

    while(1) {
        char buf[4096];
        ssize_t len = read(fd, buf, sizeof(buf));
        if(len < 0) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            error(1, errno, "pipe error");
        } else if(len == 0) {
            break;
        }

        if(writeall(tty, buf, len) != 0) {
            error(1, errno, "pipe error");
        }
    }

    if(writeall(tty, SAVE_CURSOR, strlen(SAVE_CURSOR)) != 0) {
        error(1, errno, "pipe error");
    }

    if(writeall(tty, linebuf, linelen) != 0) {
        error(1, errno, "pipe error");
    }
}

static int init_pipe_events() {
    stdpipes[STDIN_FILENO].ev = event_new(evbase, STDIN_FILENO, EV_READ | EV_PERSIST, stdin_cb, NULL);
    if(stdpipes[STDIN_FILENO].ev == NULL) {
        return -1;
    }

    if(event_add(stdpipes[STDIN_FILENO].ev, NULL) != 0) {
        return -1;
    }

    
    stdpipes[STDOUT_FILENO].ev = event_new(evbase, stdpipes[STDOUT_FILENO].fds[0], EV_READ | EV_PERSIST, stdout_cb, NULL);
    if(stdpipes[STDOUT_FILENO].ev == NULL) {
        return -1;
    }

    if(event_add(stdpipes[STDOUT_FILENO].ev, NULL) != 0) {
        return -1;
    }

    return 0;
}

int init_pipes() {
    if(init_pipe_fds() != 0) {
        return -1;
    }

    if(init_pipe_events() != 0) {
        return -1;
    }

    if(tcgetattr(tty, &tty_settings_old) != 0) {
        return -1;
    }

    if(atexit(cleanup_pipes) != 0) {
        return -1;
    }

    tty_settings = tty_settings_old;
    tty_settings.c_lflag &= ~(ICANON | ECHO);

    if(tcsetattr(tty, TCSANOW, &tty_settings) != 0) {
        return -1;
    }

    if(writeall(tty, SAVE_CURSOR, strlen(SAVE_CURSOR)) != 0) {
        return -1;
    }

    return 0;
}

int setup_fork_pipes() {
    if(dup2(stdpipes[STDIN_FILENO].fds[0], STDIN_FILENO) == -1) {
        return -1;
    }

    close(stdpipes[STDIN_FILENO].fds[0]);

    if(dup2(stdpipes[STDOUT_FILENO].fds[1], STDOUT_FILENO) == -1) {
        return -1;
    }

    if(dup2(stdpipes[STDOUT_FILENO].fds[1], STDERR_FILENO) == -1) {
        return -1;
    }

    close(stdpipes[STDOUT_FILENO].fds[1]);

    return 0;
}
