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

#include "debug.h"

void single_step_debug(pid_t pid) {
    struct user_regs_struct regs;

    ptrace(PTRACE_SINGLESTEP, pid, NULL, NULL);
    wait(NULL);

    ptrace(PTRACE_GETREGS, pid, NULL, &regs);
    peek_debug(pid, regs.rip, 64);    
}

void dump_state(pid_t pid, void *addr)
{
    struct user_regs_struct regs;

    ptrace(PTRACE_GETREGS, pid, NULL, &regs);

    print_line("===================================     CRASH DUMP     ===================================", RED);

    // just some debug stuff to trace a crash in the payload and stuff
    print_line("adr: %16lx rax: %16lx rsi: %16lx", RED, addr, regs.rax, regs.rsi);
    print_line("rip: %16lx rbx: %16lx rdi: %16lx", RED, regs.rip, regs.rbx, regs.rdi);
    print_line("r15: %16lx rcx: %16lx ", RED, regs.r15, regs.rcx);
    print_line("rax: %16lx rdx: %16lx ", RED, regs.rax, regs.rdx);

#if DEBUG
    char line[BUFSIZ];
    
    FILE *fd = proc_open(pid);
    while (fgets(line, BUFSIZ, fd) != NULL)
    {
        print_line("%s", YEL, line);
    }
    fclose(fd);

#endif
    peek_debug(pid, regs.rip, 256);

}

void clean_exit_on_sig(int sig_num)
{
    print_line("Signal %d received", RED, sig_num);

    dump_state(getpid(), NULL);
    exit(1);
}
