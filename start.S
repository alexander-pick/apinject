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

# yes I do horrible things

.intel_syntax noprefix
# This is x86-64 code!

.text
.global _entry_point                  # create global symbol
.type _entry_point, @function         # created symbol is a function

#   main:		%rdi x
#	argc:		%rsi
#	argv:		%rdx
#	init:		%rcx
#	fini:		%r8
#	rtld_fini:	%r9
#	stack_end:	stack

_entry_point:
    xor ebp, ebp
    mov r9, rdx                                 # set rtld_fini
    pop rsi
    and rsp,0xFFFFFFFFFFFFFFF0                  # proper align stack
    push rax
    push rsp
    mov rdi, launcher@GOTPCREL[rip]             # handing launcher() as main, otherwise older gcc will build a segfaulting binary
    mov rdx, 0
    mov r8, 0
	mov rcx, 0
    mov rax, __libc_start_main@GOTPCREL[rip]    # calling __libc_start_main
    call rax
    mov rax, 1                                  # exit(0), just in case, should never return here
    mov rbx, 0
    int 0x80
