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

#include "apinject.h"

void peek(pid_t pid, unsigned long addr, void *data, int len);
unsigned long poke(pid_t pid, unsigned long addr, void *data, int len);

void attach_to_pid(pid_t pid);
void get_registers(pid_t pid, struct user_regs_struct *regs);

void peek_debug(pid_t pid, unsigned long addr, int len);

void spawn_debug_monitor(pid_t target, unsigned long breakpoint);