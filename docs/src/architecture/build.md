# Build Pipeline

It took me quite a long time to truly understand what the best approach is to compile my kernel. There were a few requirements I had for myself:

1. C files should be compiled seperately from assembly files
2. Assembly should be compiled with `nasm`, C with `gcc`.
3. The boot should start at `_start`, which is in an assembly file

I do not think it is practical to have 99% in C. Assembly is great for having full control of your CPU - you know exactly what your code does. Assembly should mainly focus on booting and attaching systems to the kernel, while C should make use of the resources it gets as a kernel.

## Build Process

This had a few steps. Compiling both Assembly & C separately is straightforward, combining them into one `elf` file was quite challenging.

Here's what happens when running `make run`:

TBD
