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

#include "replacement.h"

void steal_execution(struct user_regs_struct regs)
{
    /*

    About the call convention:

    r12, r13, r14, r15, rbx, rsp, rbp are the callee-saved registers

    function parameters:
    for 64-bit x86 Linux programs the registers rdi, rsi, rcx, rdx, r8 and r9 are used
    to pass the parameters of a function.

    Linux uses SystemV ABI (ELF spec is part of that too), read more:
    https://uclibc.org/docs/psABI-x86_64.pdf (page 21)

    */

    unsigned long param_1 = regs.rdi; // param 1

    printf("param 1: %ld\n", param_1);
}
