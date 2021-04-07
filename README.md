# About

Welcome to the unofficial port of Sensible World of Soccer 96/97.
Sensible World of Soccer, or SWOS 96/97 is a soccer game developed by Sensible Software.

This is the port of the PC DOS version, developed using C++ and [SDL](https://www.libsdl.org).
Currently the only supported platforms are Windows and Android but there are plans for ports to
other platforms in the future.

Game style is PC DOS, with Amiga style currently being perfected.

## Building

For more information about building see [build.md](docs/build.md).

## Tools

There are a couple of tools that are used heavily during the development.

### ida2asm

`ida2asm` is a tool for converting pseudo-assembly output from IDA Pro into
something that can actually be assembled without errors. For more information
please see [ida2asm.md](docs/ida2asm.md).

### mnu2h

mnu2h is a mini-compiler for SWOS menu (*.mnu) files. It is converting them into C++
header files which compile directly to SWOS binary menu format. For more information
please see [mnu2h.md](docs/mnu2h.md).

## tests

For more information about tests please see [tests.md](docs/tests.md).
