pub inline fn inb(port: u16) u8 {
    return asm volatile ("inb %[port], %[ret]"
        : [ret] "={al}" (-> u8),
        : [port] "N{dx}" (port),
    );
}

pub inline fn outb(port: u16, val: u8) void {
    asm volatile ("outb %[val], %[port]"
        :
        : [port] "N{dx}" (port),
          [val] "{al}" (val),
    );
}

pub inline fn inw(port: u16) u16 {
    return asm volatile ("inw %[port], %[ret]"
        : [ret] "={ax}" (-> u16),
        : [port] "N{dx}" (port),
    );
}

pub inline fn outw(port: u16, val: u16) void {
    asm volatile ("outw %[val], %[port]"
        :
        : [port] "N{dx}" (port),
          [val] "{ax}" (val),
    );
}

pub inline fn inl(port: u16) u32 {
    return asm volatile ("inl %[port], %[ret]"
        : [ret] "={eax}" (-> u32),
        : [port] "N{dx}" (port),
    );
}

pub inline fn outl(port: u16, val: u32) void {
    asm volatile ("outl %[val], %[port]"
        :
        : [port] "N{dx}" (port),
          [val] "{eax}" (val),
    );
}
