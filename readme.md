# APInject - A Self-Contained Linux Process Injector

## What Is APInject?

APInject is a Linux process injector which is designed to inject itself into the target process, instead of using an external library. 

I wrote the project because the available injection tools didn't fit my needs and I saw some space for optimization. Somehow I like to take compilers to their limits at times, even if it is time consuming. 

The project should give you an easy to use platform for own experiments. It was written to be lightweight, easily understand- and extendable. It also has a lot of comments for those who want to play around with it. It might not conform with everyone's coding standards but that wasn't the goal here.

## A True Linux Executable And Library Hybrid

APInject is a true hybrid binary, ELF library and executable at the same time, working on latest Linux systems at the time of writing. There is very little information how to proper create such a binary available, most of it beeing outdated. 

ELF is great for such experiements but recently the developers of glibc cracked down on easy approaches. Since glibc 2.34, just creating a PIE executable isn't working anymore. `dlopen()` was merged into `glibc` and checks were added to prevent simple PIE binaries to be loaded as library. So I had to find my own way of doing it. Still the code is backwards compatible.

Instead of using PIE code I created a true hybrid binary. I implement my own start code including `__libc_start_main `loading and manually add the `.interp` section to make it work. There is no traditional `main()` in this project as it is normally initialized by the generic linked `_start ` code in `crt`.

To get into the code, look at `apinject.c` and `start.s` for a start.

## How Does The Injection Work?

The entire tool is based on `ptrace()` and leverages various fancy techniques I learned over the past years.

If the binary is run as an executable, the library loader in the start code will trigger the constructor. It checks weather the binary runs as library inside a parent or as stand alone executable.

If the binary runs as executable, `stage_one` will be executed. The code of stage_one will inject a small assembler stub into the target. The stub uses `dlopen()` to load the `apinject` binary as library into the target process. After this the program gracefully exits using `exit(0)`.

After injecting itself into the target, APInject's constructor will start running again. Inside the target `stage_two()` is executed. The constructor will search for a symbol of your choice and hook this symbol from a newly forked debugger process. 

Once the target function is executed, APInject will take over execution and allow you to run your own code prior to the original function. This allows you to dump information like passwords, manipulate variables or do what ever you want with the target process. 

The entire code should be easily adaptable by just changing target and hook symbol. Since the symbol is read from the binary, it just have to be present in `.symtab`, no `.dyntab` entry is needed.

Code which runs after the hook can be added to `replacement.c`, an explaination how to get the function arguments is included.

## Funfacts

* the injection stub is optimized and lightweight
* injection should work reliably on all x86_64 Linux systems
* compatible up to latest glibc 2.36 (which no longer has private symbols and includes dlopen now)
* handles ELF files and is able to find symbols on the fly, no need to find offsets manually
* works on C and C++ targets
* two injection techniques (page and function overwrite)
* easily configurable for a variety of tasks
* includes debug functionality for development

# Usage

The program is configured to inject into a mock service called `testsrv` and hook a function named `second_function` at the moment. 

The mock is included in this git. If you want it to hook another process just change `MY_TARGET` and `MY_TARGET_FUNC` inside `apinject.h`.

Just run ...
`make` to build APInject or the mock. 
`make run` will build and run APInject or the mock.
`make test` will build and run both

## 1. first build and run the mock 
```sh
$ cd mock
$ sudo make run
```
## 2. use another terminal to build and run APInject, requires root (!):
```sh
$ sudo make run
```

## IMPORTANT

Since this is using `ptrace()`, make sure you are running the processes as root so they have to correct permissions.

And now .. have fun!

# Misc

* released under GPLv3
* tested on Ubuntu 22.04 and 18.04

# Contact

You can contact me via twitter at `@alexander_pick` or at mastodon via `@alxhh@infosec.exchange` if you are interested to chat about the project or basebands in general.

Please fill bug reports and feature requests here at github. Valeuable pull requests are welcome if they match the license.
