# Debugging

Debugging is essential for finding bugs and truly understanding what your program does. `gdb` is great for debugging kernels. It is extremely powerful and useful tool.

Unlike regular programs where you can directly run `gdb` on the compiled binary, kernel debugging requires additional steps since we're running our code in a VM.

There are several debugging approaches:

1. Create logfiles
2. Print to the terminal (via QEMU)
3. Use `gdb`

The first two methods are similar, with the main difference being the output destination - file vs terminal. Choose based on your preference.

## GDB with QEMU

QEMU can be configured to wait for a GDB connection before executing any code, enabling debugging. Run `make debug`. A window will open & QEMU will wait for GDB to connect before starting execution

Connect GDB from another terminal:

```bash
gdb
(gdb) target remote localhost:1234
```

Load debug symbols:

```bash
(gdb) symbol-file <kernel bin>
```

Example debugging session:

```bash
gdb
(gdb) target remote localhost:1234
Remote debugging using localhost:1234
0x0000fff0 in ?? ()

(gdb) symbol-file kernel.b
Reading symbols from kernel.b...done.

(gdb) break kernel_main     # Add breakpoint to any kernel function
Breakpoint 1 at 0x101800: file kernel/kernel.c, line 12.

(gdb) continue # Qemu starts the kernel
Breakpoint 1, kernel_main (mdb=0x341e0, magic=0) at kernel/kernel.c:12
```

## Reference

- [OsDev Wiki - Kernel Debugging](https://wiki.osdev.org/Kernel_Debugging)
