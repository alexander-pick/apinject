/*
    APInject
    Copyright (C) 2022  Alexander Pick

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#ifndef HEADER_H
#define HEADER_H

#define _GNU_SOURCE 

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

#include <features.h>

#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>

#include <sys/user.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <linux/limits.h>

#include "print.h"
#include "proc.h"
#include "ptrace.h"
#include "elf.h"
#include "dlfunc.h"
#include "debug.h"

#include <dlfcn.h>

#define MY_VERSION "v0.1"
#define MY_TARGET "testsrv"
#define MY_TARGET_FUNC "second_function"

#define IS_WARRIOR 0
#define DEBUG 0

#define LIBCSTR "libc"

#define MSG_BUF_LEN 1024
#define DEBUG_PEEK 96
#define ARRSIZE 2

#define MEMBLOCKSIZE 2048
#define CHUNKSIZE 256

#define RED "\x1B[31m"
#define GRN "\x1B[32m"
#define YEL "\x1B[33m"
#define BLU "\x1B[34m"
#define MAG "\x1B[35m"
#define CYN "\x1B[36m"
#define WHT "\x1B[37m"
#define RESET "\x1B[0m"

void __attribute__((__constructor__)) we_are_in(void);

#endif // HEADER_H