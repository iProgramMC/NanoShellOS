### The NanoShell C Standard Library

#### File structure

The `include` directory contains include headers. Make
sure to use  the `-nostdinc` switch when  compiling an
application.

The `src` directory contains  all NanoShell C Standard
Library code. Please note that new files must be added
manually to the Makefile in  the directory this README
resides in.

The provided makefile has a couple functions:

`make all`:  Compiles the crti.o and crtn.o stubs, the
crt1.o entry point, and the libnanoshell.a archive for
the contents of the NanoShell C library.

`make update`: Does the same as `make all` and  copies
the libraries into system root
(`git repo root/fs/User/Library`).

`make updinc`:  Updates the include files  from `crt/`
to `git repo root/fs/User/Include`.

**Note**: The `User/` directory  is  entirely optional
and not at all used by NanoShell itself. It's required
to use TCC, though, a port of which is provided within
`apps/Tcc`. It is NanoShell's counterpart to the `usr`
directory.
