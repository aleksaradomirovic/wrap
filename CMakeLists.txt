# Copyright (C) 2025  Aleksa Radomirovic
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.31)
project(wrap LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

find_library(LIBREADLINE readline REQUIRED)
find_library(LIBHISTORY history REQUIRED)

add_executable(
    wrap
        src/poll.c
        src/program.c
        src/readline.c
        src/wrap.c
        src/wrap.h
)
target_link_libraries(
    wrap
    PRIVATE
        ${LIBREADLINE}
        ${LIBHISTORY}
)
