#![no_std]

use core::panic::PanicInfo;

mod vga;
pub use vga::{vga_clear, vga_putchar, vga_puts};

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}
