# genbigint

Generate ASM, C and C++ source code for fixed size big integer arithmetic and bitwise operations.

The generator is written in plain C, so it can be built and ran on any platform.
The generated ASM code is using Intel syntax. I used NASM to assemble, but other assemblers might work just fine.

There is an optional solution and project for Visual Studio 2022 for building the generator on Windows.
There are some cmd scripts to generate, build and run some tests (require nasm.exe to be in the PATH).

Some operations (e.g. add, subtract etc.) are done 100% in ASM, while others (multiply, divide) are done partly in ASM, partly in C.

The C++ implementation is a wrapper of the ASM or C functions.
