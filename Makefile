# please don't hate me for my fugly makefiles :-)

CC = gcc

asmflags =  -O0 -nostartfiles --entry payload_loader -nostdlib -nodefaultlibs -c -m64
# asm flags are pretty much set that there is no bloat in the obj

compflags = -fPIC -O0 -c -Wunused-variable -m64
linkflags = -lc -fPIC -O0 -Wall -m64 -z noexecstack -no-pie -shared -Wl,-e _entry_point 

# Important params:
# -fPIC 				- self explanatory 
# -shared 				- new systems need dynamic libaries and won't load simple PIE objects with dlopen
# -no-pie 				- new systems don't like PIE
# -z noexecstack 		- prevent an executable stack and crashes caused by linking a asm obj
# -Wl,-e _entry_point 	- this is the entry point in my self implemented start.s

# Note: you need to compile the injector.s seperatly or your will get a binary
# causing segfaults if it is injected in some cases.

# Note 2: found out why, compiling the assembly and linking it somehow makes 
# the stack of the injector executable, need to set -z noexecstack to fix that
# testcase for this issue can ebe found here: https://github.com/alexander-pick/ld-assembler-link-issue

all:
		${CC} $(asmflags) -o injector.o injector.s
		${CC} $(compflags) apinject.c -o apinject.o
		${CC} $(compflags) print.c -o print.o
		${CC} $(compflags) proc.c -o proc.o
		${CC} $(compflags) ptrace.c -o ptrace.o
		${CC} $(compflags) dlfunc.c -o dlfunc.o
		${CC} $(compflags) replacement.c -o replacement.o 
		${CC} $(compflags) elf.c -o elf.o
		${CC} $(compflags) debug.c -o debug.o
		${CC} $(linkflags) start.s injector.o dlfunc.o print.o proc.o ptrace.o elf.o replacement.o debug.o apinject.o -o apinject
clean:
		rm -f apinject
		rm -f *.o
run: 
		make all
		./apinject
test: 
		make all
	  	cd ./mock && make
	  	cd ..
	  	./mock/testsrv &
	  	./apinject