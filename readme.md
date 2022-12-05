# APInject - A Self-Contained Linux Process Injector

## What is APInject?

APInject is a Linux process injector which is designed to inject itself into the target process, instead of using an external library. 

I wrote the project because the available injection tools didn't fit my needs and I saw some space for optimization. The project should give you an easy to use platform for own experiments. It was written to be lightweight, easily understand- and extendable. It might not conform with everyone's coding standards but that wasn't the goal here.

## How does it work?

APInject will inject a small stub into the target to load the `apinject` binary into the target process. After injecting itself into the target, APInject's constructor will start running. The constructor will search for a symbol of your choice and hook this symbol from a newly forked debugger process. 

Once the target function is executed, APInject will take over execution and allow you to run your code prior to the original function. This allows you to dump information like passwords, manipulate variables or do what ever you want with the target process. The entire tool is based on `ptrace()` and leverages various fancy techniques I learned over the past years.

## Funfacts

* the injection stub is optimized and lightweight
* injection should work reliably on all x86_64 Linux systems
* only uses glibc's private symbols for injection, no `libdlopen` requried
* compatible up to latest glibc 2.36 (which no longer has private symbols and includes dlopen now)
* handles ELF files and is able to find symbols on the fly, no need to find offsets manually
* works on C and C++ targets
* two injection techniques (page and function overwrite)
* flexible configurable for a variety of tasks
* includes debug functionality for development

# Usage

The program is configured to inject into a mock service called `testsrv` and hook a function named `second_function` at the moment. The mock is included in this git. If you want it to hook another process just change `MY_TARGET` and `MY_TARGET_FUNC` inside `apinject.h`.

Just run ...
`make` to build APInject or the mock. 
`make run` will build and run APInject or the mock.

* first build and run the mock 
```sh
$ cd mock
$ make run
```
* use another terminal to build and run APInject, requires root (!):
```sh
# make run
```

Have fun!

# Misc

* released under GPLv3
* tested on Ubuntu 22.04 and 18.04

# Contact

You can contact me via twitter at `@alexander_pick` or at mastodon via `@alxhh@infosec.exchange` if you are interested to chat about the project or basebands in general.

Please fill bug reports and feature requests here at github. Valeuable pull requests are welcome if they match the license.