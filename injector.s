#    APInject
#    Copyright (C) 2022  Alexander Pick
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

# This file implements: void payload(void *data[])
# it loads our code as library to the target adress space, hammer and chisel implementation

.intel_syntax noprefix
# This is x86-64 code!

.text
.global payload_loader                  # create global symbol
.type payload_loader, @function         # created symbol is a function

# can be debugged by using this (gdb won't work as we use ptrace):
# export LD_DEBUG=all

payload_loader:
        nop                         # these protect us from weird shifting issues
        nop
        call .Lget_rip              # quickly snitch the value of rip

        sub rax, 0x107 # 105        # calculate a pointer to *data[] is in rax, data is located at 
                                    # -254 bytes (0x100) from current rip and off by 5 because 
                                    # of the call opcode length
        mov r15, rax                # store result in r15 for further use

        and rsp,0xFFFFFFFFFFFFFFF0  # we need this call to archive a 16 byte boundary alignment and prevent a crash during
                                    # relocation. glibc is optimized using movaps and if the stack is not correctly aligned
                                    # to a 16 byte boundary these SIMD ins will just GP fault (see Intel manual for details 
                                    # on movaps)  
        
        mov rbx, [r15]              # get __libc_dlopen_mode address into rbx

        mov rdi, r15                # mov base of data to edi
        add rdi, 16                 # add +16 to point to make rdi contain pointer to the path 

        mov rsi, 0x00102            # RTLD_NOW | RTLD_GLOBAL 
        
        call rbx                    # __libc_dlopen_mode

        int 0x3                     # breakoint to resume execution

.Lget_rip: 
        mov rax, [rsp]
        ret
