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

#include "dlfunc.h"

/* some globals */
unsigned long libc_handle;
unsigned long proc_lib;
unsigned long local_lib;

// https://elixir.bootlin.com/glibc/latest/source/elf/dl-libc.c
#if defined(__GLIBC__) && __GLIBC__ == 2 && __GLIBC_MINOR__ < 34
extern void *__libc_dlopen_mode(const char *name, int mode);

void *local__dlopen(const char *name, int mode) {
#if DEBUG    
    print_line("calling __libc_dlopen_mode", YEL);
#endif
   return __libc_dlopen_mode(name, mode);
}
#else
void *(*hidden_libc_dlopen_mode)(const char *__file, int __mode) __THROWNL;

void *local__dlopen(const char *name, int mode) {
#if 1    
    print_line("calling dlopen", YEL);
#endif
    pid_t target_pid = proc_get_pid(MY_TARGET, false);

    print_line("pid: %d", YEL, target_pid);
    
    char *filename = (char *)proc_get_libc_name(target_pid);

    print_line("filename: %s", YEL, filename);
    
    unsigned long dlopen_addr = get_offset_from_elf(filename, "dlopen");

    print_line("dlopen: %p", YEL, dlopen_addr);

    hidden_libc_dlopen_mode = (void *)dlopen_addr;

    print_line("calling __libc_dlopen_mode at %p", YEL, *hidden_libc_dlopen_mode);
    return (hidden_libc_dlopen_mode)(name, mode);
}
#endif

// https://elixir.bootlin.com/glibc/latest/source/elf/dl-libc.c
#if defined(__GLIBC__) && __GLIBC__ == 2 && __GLIBC_MINOR__ < 34
extern void *__libc_dlsym(void *map, const char *name);

void *local__dlsym(void *map, const char *name) {
#if DEBUG     
    print_line("calling __libc_dlsym", YEL);
#endif
    return __libc_dlsym(map, name);
}
#else
extern void *dlsym(void *map, const char *name);

void *local__dlsym(void *map, const char *name) {
#if DEBUG    
    print_line("calling dlsym", YEL);
#endif
    dlsym(map, name);
}
#endif

extern void *_dl_sym (void *handle, const char *name, void *who);

unsigned long get_func_local(char *symbol)
{
    void *sym_addr = local__dlsym((void *)libc_handle, symbol); //(void *)libc_handle

    if (sym_addr == NULL)
    {

        print_line("error locating %s()", RED, symbol);
        exit(1);
    }
    else
    {

        print_line("%s() local %p", BLU, symbol, sym_addr);
    }

    return (unsigned long)sym_addr;
}

unsigned long get_func_remote(char *symbol)
{
    print_line("get_func_remote: %s", GRN, symbol);
    unsigned long sym_addr = (unsigned long)local__dlsym((void *)libc_handle, symbol); 

    if (sym_addr == 0)
    {

        print_line("error locating %s()", RED, symbol);
        exit(1);
    }
    else
    {

        print_line("found local %s() at %p", BLU, symbol, sym_addr);
    }

#if DEBUG
    print_line("local_lib:%p sym_addr:%p proc_lib:%p", YEL, local_lib, sym_addr, proc_lib);
#endif

    sym_addr = proc_lib + (sym_addr - local_lib);

    print_line("found target %s() at %p", BLU, symbol, sym_addr);

    return (unsigned long)sym_addr;
}

void get_libc_addrs(pid_t pid)
{
    libc_handle = (unsigned long)local__dlopen(proc_get_libc_name(-1), 1); // 1=RTLD_LAZY

    if ((void *)libc_handle == NULL)
    {

        print_line("error loading %s", RED, LIBCSTR);
        exit(1);
    }
    local_lib = proc_get_image(-1, LIBCSTR, 0, false);
    proc_lib = proc_get_image(pid, LIBCSTR, 0, false);
}