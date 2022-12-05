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

#.section 
.text                          # text section
#.align 2;
.global payload_loader                  # create global symbol
#.type _ayload_loader, @function         # created symbol is a function
# This is x86-64 code!

# debug by using this (gdb won't work as we use ptrace):
# export LD_DEBUG=all

.macro pushaq
    push rsp                    # stack pointer first
    push rbp
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rdx    
    push rcx
    push rbx
 #   push rax                   # exclude accumulator
    push rsi
    push rdi
.endm # pushaq

.macro popaq
    pop rdi
    pop rsi
 #   pop rax
    pop rbx
    pop rcx
    pop rdx
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15       
    pop rbp
    pop rsp
.endm

# this is just some macro which can be used for quick and stupid print debugging.
# if you not uncomment the lines below, macro shouldn't be included in the final
# binary, that makes it more convident for me than functions
.macro debug_print
        pushaq
        push rax
        mov  rax, 0x0a786148646162  # put badHax on stack
        push rax
        mov  rdx, 0x8               # atack string length
        mov  rsi, rsp               # put addr of the stack string into rsi
        mov  rax, 0x1               # syscall 1 is write()
        mov  rdi, 0x1               # stdout = fd 1
        syscall                     # call kernel
        pop rax
        pop rax
        popaq
.endm

.macro debug_print_path
    pushaq
    mov rdx, [r15+8]            # length
    mov rsi, r15                # add base of data to rsi
    add rsi, 16                 # add +16 to point to the path 
    mov rax, 1                  # syscall number
    mov rdi, 1                  # stdout = fd 1
    syscall                     # call kernel   
    popaq
.endm

payload_loader:
        nop                         # these protect us from weird shifting issues
        nop
        call .Lget_rip              # quickly snitch the value of rip

        # xor ebp,ebp
        # push rax
        # push rsp
        # push rbp
        # mov rbp, rsp

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

        mov rsi, 1 # 0x00102            # RTLD_NOW | RTLD_GLOBAL 

        # debug_print_path
        
        call rbx                    # __libc_dlopen_mode

        int 0x3                     # breakoint to resume execution

.Lget_rip: 
        mov rax, [rsp]
        ret
