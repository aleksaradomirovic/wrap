# Copyright (C) 2025  Aleksa Radomirovic
# SPDX-License-Identifier: GPL-3.0-or-later

project(
    'wrap',
    'c',
    default_options: { 'b_lto': true, 'c_std': 'gnu23', 'buildtype': 'release' }
    )

readline = dependency('readline', required: true)

wrap = executable(
    'wrap',
    'main.c', 'wrap.c', 'wrap.h',
    dependencies: readline,
    install: true
    )
