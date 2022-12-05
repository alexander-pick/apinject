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

#include "ptrace.h"

/*
    peek memory
*/

void peek(pid_t pid, unsigned long addr, void *data, int len)
{
    unsigned long word = 0x0;
    int word_length = sizeof(unsigned long);

    for (int i = 0; i < len; i += word_length)
    {
        word = ptrace(PTRACE_PEEKDATA, pid, (void *)(addr + i), NULL);

        if (word == -1)
        {
            print_line("Error reading process memory (0x%llx)\n", RED, addr);
            exit(1);
        }

        memcpy((data + i), &word, word_length);
    }
}

/*
    poke memory, len has to be % 8 = 0 (this means word aligned)
*/

unsigned long poke(pid_t pid, unsigned long addr, void *data, int len)
{
    unsigned long word = 0x0;
    int word_length = sizeof(unsigned long);

    for (int i = 0; i < len; i += word_length)
    {
        memcpy(&word, (data + i), word_length);

        if (ptrace(PTRACE_POKEDATA, pid, (void *)(addr) + i, word) == -1)
        {
            print_line("Error writing process memory (0x%lx)\n", RED, addr);
            exit(1);
        }
#if 0
        print_line("DEBUG: wrote %lx", YEL, word);
#endif
    }

    return (addr + (unsigned long)len);
}

/*
    attach to target PID
*/

void attach_to_pid(pid_t pid)
{

    print_line("attaching to %d", GRN, pid);

    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0)
    {

        print_line("ptrace attach failed!", RED);
        exit(1);
    }

    wait(NULL); // wait for attach

    long ret = ptrace(PTRACE_SETOPTIONS, pid, NULL, PTRACE_O_TRACECLONE);
    print_line("ptrace setoptions (ret:%d)", GRN, ret);
}

/*
    get all registers
*/

void get_registers(pid_t pid, struct user_regs_struct *regs)
{

    print_line("getting registers", GRN);

    if (ptrace(PTRACE_GETREGS, pid, NULL, regs) < 0)
    {

        print_line("ptrace getregs failed!", RED);
        exit(1);
    }
}

// look at replacement.cpp for this one
extern void steal_execution(struct user_regs_struct regs);

/*
    this is essentially a debugger which will breakpoint the target function
    and highjack execution to do fancy stuff
*/

void spawn_debug_monitor(pid_t target, unsigned long breakpoint)
{

    int status;
    struct user_regs_struct regs;

    // sleep a bit to wait for the injection to finish
    // and ptrace to be free for reattach from here
    sleep(1);

    attach_to_pid(target);

    print_line("setting breakpoint at %p", GRN, breakpoint);

    int word_length = sizeof(unsigned long);

    unsigned char *saved_opcode = malloc(word_length);

    peek(target, breakpoint, saved_opcode, word_length); // peek and poke require word length

    unsigned char *int3_opcode = malloc(word_length);

    memcpy(int3_opcode, saved_opcode, word_length);
    memcpy(int3_opcode, "\xCC", 1);

    poke(target, breakpoint, int3_opcode, word_length);

    ptrace(PTRACE_CONT, target, NULL, 0);

    while (1)
    {

        pid_t current_pid = waitpid(target, &status, WUNTRACED | WNOHANG);

        if (WIFSTOPPED(status) &&
            WSTOPSIG(status) == SIGTRAP)
        {

            if (current_pid != target)
            {
                continue;
            }

            print_line("SIGTRAP recieved", GRN); // SIG 0x5

            get_registers(target, &regs);

            // here we run our own code
            steal_execution(regs);

            print_line("restoring original code", BLU);
            poke(target, breakpoint, saved_opcode, word_length);

            print_line("restoring rip to original position", BLU);
            regs.rip = breakpoint;
            ptrace(PTRACE_SETREGS, target, NULL, &regs);

            print_line("single stepping...", GRN);
            ptrace(PTRACE_SINGLESTEP, target, NULL, NULL);
            wait(NULL);

            print_line("resetting the breakpoint for next iteration", GRN);
            poke(target, breakpoint, int3_opcode, word_length);

            // continue the process
            ptrace(PTRACE_CONT, target, NULL, 0);
        }

        if (WIFSTOPPED(status))
        {
            // continue the process if it has stopped for some reason
            ptrace(PTRACE_CONT, target, NULL, 0);
        }
    }
}   
