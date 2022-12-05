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

/*
    Constructor which is fully run once we injected ourself into
    the target process. This is the entry point inside the target
    process. See this as the second main(), just inside the target.
*/

void __attribute__((__constructor__)) we_are_in(void)
{
    print_line("constructor executed", YEL);

    pid_t mbts_pid;

    char myself[BUFSIZ];

    mbts_pid = proc_get_pid(MY_TARGET, false);

    // check if we are already injected into target

    if (mbts_pid == getpid())
    {

        print_line("Here's Johnny!!!", YEL);

        if (!readlink("/proc/self/exe", myself, BUFSIZ))
        {
            print_line("readlink failed!", RED);
            exit(1);
        }

        print_line("whoami (%s)", GRN, myself);

        print_line("getting symbol offset", GRN);
        long unsigned func_offset = get_offset_from_elf(myself, MY_TARGET_FUNC);

        if (func_offset == 0)
        {
            print_line("no offset found", RED);
            exit(1);
        }

        print_line("getting image base in memory", GRN);

        long unsigned target_image = proc_get_image(-1, MY_TARGET, 0, false);

        // fork and spawn a debugger child which debugs the main thread and
        // controls execution
        if (fork() == 0)
        {

            // here we are in the new child
            print_line("spawned debug child (pid: %d)", GRN, getpid());

            spawn_debug_monitor(mbts_pid, (target_image + func_offset));
        }

        // we are done in the main thread
        print_line("done (%d).", GRN, getpid());
    }
}

// extern for asm stub
extern void payload_loader();

int main(int argc, char *argv[])
{
    struct user_regs_struct regs;        // register dump of mbts
    struct user_regs_struct backup_regs; //

    pid_t mbts_pid;  // pid of mbts
    int mbts_status; // status after waitpid/break

    unsigned long data[ARRSIZE];

    char myself[BUFSIZ];
    unsigned long myself_length;

    unsigned long target_addr;
    unsigned char *block_backup;

    unsigned long current_offset = 0;

    print_info();

    signal(SIGSEGV, clean_exit_on_sig); 

    mbts_pid = proc_get_pid(MY_TARGET, false);

    if (mbts_pid == 0)
    {
        print_line("unable to find PID of target", RED);
        exit(0);
    }

    if (mbts_pid == getpid())
    {
        exit(0);
    }

    print_line("target has PID %i", GRN, mbts_pid);

    get_libc_addrs(mbts_pid);

#if defined(__GLIBC__) && __GLIBC__ == 2 && __GLIBC_MINOR__ < 34
    data[0] = get_func_remote("__libc_dlopen_mode");
#else
    data[0] = get_func_remote("dlopen");
#endif

    attach_to_pid(mbts_pid);

    get_registers(mbts_pid, &regs);

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
    target_addr = proc_get_image(mbts_pid, "", 0, false);
#endif

    if (target_addr == 0)
    {

        print_line("no executable page found", YEL);
        exit(1);
    }

    // backup current register content
    print_line("backing up previous content (%p)", GRN, (void *)target_addr);
    block_backup = (unsigned char *)malloc(MEMBLOCKSIZE);
    peek(mbts_pid, target_addr, block_backup, MEMBLOCKSIZE);

#if DEBUG
    peek_debug(mbts_pid, target_addr, DEBUG_PEEK);
#endif

    // adding our data at base
    print_line("adding our data to remote memory (%p)", GRN, (void *)target_addr);
    current_offset = poke(mbts_pid, target_addr, (void *)&data, (ARRSIZE * sizeof(unsigned long)));

#if DEBUG
    peek_debug(mbts_pid, target_addr, DEBUG_PEEK);
#endif

    // add string with the path to the library right after our data
    print_line("adding our loader string to remote memory (%p len:%lld)", GRN, (void *)current_offset, myself_length);
    current_offset = poke(mbts_pid, current_offset, myself, myself_length);

#if DEBUG
    peek_debug(mbts_pid, target_addr, DEBUG_PEEK);
#endif
    void *loader_addr = &payload_loader;
    print_line("injecting loader (%p)", GRN, loader_addr);
    poke(mbts_pid, (target_addr + CHUNKSIZE), loader_addr, CHUNKSIZE);

#if DEBUG
    peek_debug(mbts_pid, (target_addr + CHUNKSIZE), DEBUG_PEEK);
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
    ptrace(PTRACE_SETREGS, mbts_pid, NULL, &regs);

#if 0
    for(int s = 0;s < 30; s++) {
        single_step_debug(mbts_pid);
    }
#endif
    
    print_line("continuing target prcocess (%ld)", GRN, mbts_pid);
    ptrace(PTRACE_CONT, mbts_pid, NULL, NULL);
    waitpid(mbts_pid, &mbts_status, WUNTRACED); // catch SIGTRAP from 0x3

    if (WIFSTOPPED(mbts_status) && WSTOPSIG(mbts_status) == SIGTRAP)
    {
        print_line("SIGTRAP recieved", GRN); // SIG 0x5
        ptrace(PTRACE_GETREGS, mbts_pid, NULL, &regs);

        print_line("dlopen returned status %lx", GRN, regs.rax);

    }
    else
    {

        print_line("something went wrong, we recieved status %d", RED, WSTOPSIG(mbts_status));

#if 1
        dump_state(mbts_pid, (void *)future_rip);
#endif
    }

    // restore memory of our target
    poke(mbts_pid, target_addr, block_backup, MEMBLOCKSIZE);

    // restore regs
    ptrace(PTRACE_SETREGS, mbts_pid, NULL, &backup_regs);

    ptrace(PTRACE_DETACH, mbts_pid, NULL, NULL);

    print_line("mission accomplished", CYN);

    return 0;
}
