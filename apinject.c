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

// injector stages
void stage_one();
void stage_two(pid_t pid);

const char interp_path[] __attribute__((section(".interp"))) = "/lib64/ld-linux-x86-64.so.2";

/*
    This is the constructor which launches the two stages of the loader: 
    stage one: setup the shellcode and execute it in targets adress space
    stage two: constructor run after dlopen loaded the binary into the target
*/

void launcher() __attribute__((__constructor__));
void launcher()
{
    pid_t pid = proc_get_pid(MY_TARGET, false);

    // check if we are already injected into target

    if (pid == getpid())
    {
        stage_two(pid);
    }
    else
    {
        stage_one();
    }
}

/*
    this is "main()"
*/ 
int kickstarter() {
    launcher();
}

// extern for asm stub
extern void payload_loader();

void stage_one()
{
    struct user_regs_struct regs;        // register dump of mbts
    struct user_regs_struct backup_regs; //

    pid_t pid;  // pid of mbts
    int mbts_status; // status after waitpid/break

    unsigned long data[ARRSIZE];

    char myself[BUFSIZ];
    unsigned long myself_length;

    unsigned long target_addr;
    unsigned char *block_backup;

    unsigned long current_offset = 0;

    print_info();

    // set handler for segfaults
    signal(SIGSEGV, clean_exit_on_sig);

    // get PID of target
    pid = proc_get_pid(MY_TARGET, false);

    if (pid == 0)
    {
        print_line("unable to find PID of target", RED);
        exit(0);
    }

    if (pid == getpid())
    {
        exit(0);
    }

    print_line("target has PID %i", GRN, pid);

    // get libc addr
    get_libc_addrs(pid);

#if defined(__GLIBC__) && __GLIBC__ == 2 && __GLIBC_MINOR__ < 34
    data[0] = get_func_remote("__libc_dlopen_mode");
#else
    data[0] = get_func_remote("dlopen");
#endif

    // ptrace attach
    attach_to_pid(pid);

    // get registers
    get_registers(pid, &regs);

    // backup all regs for later restore
    backup_regs = regs;

    if (!readlink("/proc/self/exe", myself, BUFSIZ))
    {
        print_line("readlink failed!", RED);
        exit(1);
    }

    print_line("whoami (%s)", GRN, myself);
    myself_length = strlen(myself);

    data[1] = myself_length;

#if IS_WARRIOR
    /*
        -> this is the way of the warrior ... arrrrr

        let's write over a function in libc not needed for our purpose and which is unlikely to run
        at injection time. function will be for sure located in executable memory, we will restore
        it once done
    */

    target_addr = get_func_remote("ntp_gettime");
#else
    /*
        -> this is the boring way which uses proc to find an exacutable page to write to
    */
    target_addr = proc_get_image(pid, "", 0, false);
#endif

    if (target_addr == 0)
    {

        print_line("no executable page found", YEL);
        exit(1);
    }

    // backup current register content
    print_line("backing up previous content (%p)", GRN, (void *)target_addr);
    block_backup = (unsigned char *)malloc(MEMBLOCKSIZE);
    peek(pid, target_addr, block_backup, MEMBLOCKSIZE);

#if DEBUG
    peek_debug(pid, target_addr, DEBUG_PEEK);
#endif

    // adding our data at base
    print_line("adding our data to remote memory (%p)", GRN, (void *)target_addr);
    current_offset = poke(pid, target_addr, (void *)&data, (ARRSIZE * sizeof(unsigned long)));

#if DEBUG
    peek_debug(pid, target_addr, DEBUG_PEEK);
#endif

    // add string with the path to the library right after our data
    print_line("adding our loader string to remote memory (%p len:%lld)", GRN, (void *)current_offset, myself_length);
    current_offset = poke(pid, current_offset, myself, myself_length);

#if DEBUG
    peek_debug(pid, target_addr, DEBUG_PEEK);
#endif
    void *loader_addr = &payload_loader;
    print_line("injecting loader (%p)", GRN, loader_addr);
    poke(pid, (target_addr + CHUNKSIZE), loader_addr, CHUNKSIZE);

#if DEBUG
    peek_debug(pid, (target_addr + CHUNKSIZE), DEBUG_PEEK);
#endif

    /*
        layout at this point:

        base            +-----------+ 0
                        | dlopen    |
                        +-----------+ 8
                        | path len  |
                        +-----------+ 16
                        | path      |
                        ,           ,
                        .           .

        base + 256      +-----------+
                        | payload   |
                        ,           ,
                        .           .
    */

    unsigned long future_rip = (target_addr + CHUNKSIZE + 2); //+2

    print_line("pointing rip to payload at %p", GRN, future_rip);
    // move instruction pointner to payload
    regs.rip = future_rip; // rip = offset of future payload
    ptrace(PTRACE_SETREGS, pid, NULL, &regs);

#if 0
    for(int s = 0;s < 30; s++) {
        single_step_debug(pid);
    }
#endif

    print_line("continuing target process (%ld)", GRN, pid);
    ptrace(PTRACE_CONT, pid, NULL, NULL);
    waitpid(pid, &mbts_status, WUNTRACED); // catch SIGTRAP from 0x3

    if (WIFSTOPPED(mbts_status) && WSTOPSIG(mbts_status) == SIGTRAP)
    {
        print_line("SIGTRAP recieved", GRN); // SIG 0x5
        ptrace(PTRACE_GETREGS, pid, NULL, &regs);

        print_line("dlopen returned status %lx", GRN, regs.rax);
    }
    else
    {

        print_line("something went wrong, we recieved status %d", RED, WSTOPSIG(mbts_status));

#if DEBUG
        dump_state(pid, (void *)future_rip);
#endif
    }

    // restore memory of our target
    poke(pid, target_addr, block_backup, MEMBLOCKSIZE);

    // restore regs
    ptrace(PTRACE_SETREGS, pid, NULL, &backup_regs);

    ptrace(PTRACE_DETACH, pid, NULL, NULL);

    print_line("mission accomplished", CYN);
    exit(0);
}

void stage_two(pid_t pid)
{

    // fork and spawn a debugger child which debugs the main thread and
    // controls execution
    if (fork() == 0)
    {
        // here we are in the new child
        print_line("spawned debug child (pid: %d)", GRN, getpid());

        //find out who we are
        char myself[BUFSIZ];

        if (!readlink("/proc/self/exe", myself, BUFSIZ))
        {
            print_line("readlink failed!", RED);
            exit(1);
        }

        print_line("whoami (%s)", GRN, myself);

        // extract symbol location from .symtbl (not present in the mem image)
        long unsigned symbol_offset = get_offset_from_elf(myself, MY_TARGET_FUNC);

        if (symbol_offset == 0)
        {
            print_line("no offset found", RED);
            exit(0);
        }

        print_line("symbol offset %p", GRN, (void *)symbol_offset);

        long unsigned target_image = proc_get_base(pid);

        long unsigned func_offset = target_image + symbol_offset;

        print_line("function offset %p ", GRN, func_offset);

        // enter the debug monitor
        spawn_debug_monitor(pid, func_offset);
    }

    // we are done in the main thread
    print_line("done (%d).", GRN, getpid());
}
