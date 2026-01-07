const VGA_MEMORY: u32 = 0xc00b8000;
pub const VGA_WIDTH: usize = 80;
pub const VGA_HEIGHT: usize = 25;

use core::fmt;
use lazy_static::lazy_static;
use spin::Mutex;

lazy_static! {
    pub static ref WRITER: Mutex<Writer> = Mutex::new(Writer {
        column: 0,
        row: VGA_HEIGHT - 1,
        colour: ColourCode::new(VgaColour::White, VgaColour::Black),
        buffer: unsafe { &mut *(VGA_MEMORY as *mut Buffer) },
    });
}

#[derive(Copy, Clone)]
pub struct ColourCode(u8);

#[repr(u8)]
pub enum VgaColour {
    Black = 0,
    Blue = 1,
    Green = 2,
    Cyan = 3,
    Red = 4,
    Magenta = 5,
    Brown = 6,
    LightGrey = 7,
    DarkGrey = 8,
    LightBlue = 9,
    LightGreen = 10,
    LightCyan = 11,
    LightRed = 12,
    LightMagenta = 13,
    Yellow = 14,
    White = 15,
}

impl ColourCode {
    pub fn new(fg: VgaColour, bg: VgaColour) -> ColourCode {
        ColourCode(((bg as u8) << 4) | (fg as u8))
    }
}

#[repr(C)]
#[derive(Copy, Clone)]
pub struct VgaChar {
    pub ascii_character: u8,
    pub colour: ColourCode,
}

#[repr(transparent)]
pub struct Buffer {
    pub chars: [[VgaChar; VGA_WIDTH]; VGA_HEIGHT],
}

pub struct Writer {
    column: usize,
    row: usize,
    colour: ColourCode,
    buffer: &'static mut Buffer,
}

impl fmt::Write for Writer {
    fn write_str(&mut self, s: &str) -> fmt::Result {
        self.write_string(s);
        Ok(())
    }
}

impl Writer {
    pub fn write_string(&mut self, str: &str) {
        for byte in str.bytes() {
            match byte {
                0x20..=0x7e | b'\n' => self.write_byte(byte),
                _ => self.write_byte(0xfe),
            }
        }
    }

    fn write_byte(&mut self, byte: u8) {
        match byte {
            b'\n' => self.new_line(),
            byte => {
                if self.column >= VGA_WIDTH {
                    self.new_line();
                }

                let row = self.row;
                let col = self.column;

                self.buffer.chars[row][col] = VgaChar {
                    ascii_character: byte,
                    colour: self.colour,
                };

                self.column += 1;
            }
        }
    }

    fn shift_lines_up(&mut self) {
        for row in 1..VGA_HEIGHT {
            for col in 0..VGA_WIDTH {
                let character = self.buffer.chars[row][col];
                self.buffer.chars[row - 1][col] = character;
            }
        }

        let blank = VgaChar {
            ascii_character: b' ',
            colour: self.colour,
        };
        for col in 0..VGA_WIDTH {
            self.buffer.chars[VGA_HEIGHT - 1][col] = blank;
        }
    }

    #[inline]
    fn new_line(&mut self) {
        self.column = 0;
        self.shift_lines_up();
    }

    pub fn clear_screen(&mut self) {
        self.column = 0;
        self.row = VGA_HEIGHT - 1;

        let blank = VgaChar {
            ascii_character: b' ',
            colour: self.colour,
        };
        self.buffer.chars = [[blank; VGA_WIDTH]; VGA_HEIGHT];
    }

    pub fn clear_line(&mut self) {
        self.column = 0;
        let blank = VgaChar {
            ascii_character: b' ',
            colour: self.colour,
        };

        for col in 0..VGA_WIDTH {
            self.buffer.chars[VGA_HEIGHT - 1][col] = blank;
        }
    }

    pub fn clear_char(&mut self) {
        self.column -= 1;

        let row = self.row;
        let column = self.column;

        let blank = VgaChar {
            ascii_character: b' ',
            colour: self.colour,
        };
        self.buffer.chars[row][column] = blank;
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn vga_putchar(c: u8) {
    WRITER.lock().write_byte(c);
}

#[unsafe(no_mangle)]
pub extern "C" fn vga_puts(s: *const u8) {
    if s.is_null() {
        return;
    }

    unsafe {
        let mut ptr = s;
        while *ptr != 0 {
            WRITER.lock().write_byte(*ptr);
            ptr = ptr.add(1);
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn vga_clear() {
    WRITER.lock().clear_screen();
}
