# Copyright (C) 2025  Aleksa Radomirovic
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.31)
project(wrap LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

option(WRAP_USE_SYSTEM_OPTIMIZATIONS "Use OS-specific (non-POSIX) optimizations" ON)

add_executable(
    wrap
        src/fork_program.c
        src/io_loop.c
        src/pipes.c
        src/run_program.c
        src/wrap.c
        src/wrap.h
)
target_compile_definitions(
    wrap
    PRIVATE
        $<$<AND:$<BOOL:${WRAP_USE_SYSTEM_OPTIMIZATIONS}>,$<STREQUAL:${CMAKE_SYSTEM_NAME},Linux>>:_GNU_SOURCE=1>
)
