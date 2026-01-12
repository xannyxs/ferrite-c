target remote :1234
add-symbol-file "kernel/kernel.elf" 0x100000

set disassembly-flavor intel
set architecture i386

layout src
b kmain
