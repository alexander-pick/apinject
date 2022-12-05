# please don't hate me for my fugly makefiles

CC = gcc

asmflags =  -O -nostartfiles --entry payload_loader -nostdlib -nodefaultlibs -c
compflags = -O -c -fPIC -Wunused-variable -m64
linkflags = -fPIE -fPIC -Wall -m64 -lc -z noexecstack

# Note: you need to compile the .s seperatly or your will have a binary
# causing segfaults if it is injected.
# Note 2: found out why, compiling the assembly and linking it somehow makes 
# the stack of the injector executable, need to set -z noexecstack to fix that

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
		${CC} $(linkflags) injector.o dlfunc.o print.o proc.o ptrace.o elf.o replacement.o debug.o apinject.o -o apinject
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