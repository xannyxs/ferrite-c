#include <drivers/serial.h>
#include <drivers/vga.h>
#include <ferrite/types.h>

#define VGA_ENTRY_COLOUR(fg, bg) ((u8)(fg) | ((u8)(bg) << 4))
#define VGA_ENTRY(c, colour) ((u16)(c) | ((u16)(colour) << 8))

#define WRITER_NEW                                                  \
    { .column = 0,                                                  \
      .row = 0,                                                     \
      .colour = VGA_ENTRY_COLOUR(VGA_COLOR_WHITE, VGA_COLOR_BLACK), \
      .buffer = (volatile u16*)VGA_MEMORY }

static Writer WRITER = WRITER_NEW;

/* Private */

static void writer_scroll(Writer* self)
{
    for (size_t y = 1; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            size_t src_index = y * VGA_WIDTH + x;
            size_t dst_index = (y - 1) * VGA_WIDTH + x;
            self->buffer[dst_index] = self->buffer[src_index];
        }
    }

    u16 blank = VGA_ENTRY(' ', self->colour);
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        size_t index = (VGA_HEIGHT - 1) * VGA_WIDTH + x;
        self->buffer[index] = blank;
    }
}

static void writer_new_line(Writer* self)
{
    self->column = 0;
    if (self->row < VGA_HEIGHT - 1) {
        self->row += 1;
        return;
    }

    writer_scroll(self);
}

static void writer_write_char(Writer* self, u8 c)
{
#if defined(__print_serial)
    serial_write_byte(c);
#endif

    if (c == '\n') {
        writer_new_line(self);
        return;
    }

    size_t index = self->row * VGA_WIDTH + self->column;
    self->buffer[index] = VGA_ENTRY(c, self->colour);
    self->column += 1;

    if (self->column >= VGA_WIDTH) {
        writer_new_line(self);
    }
}

static void writer_write_string(Writer* self, char const* str)
{
    for (size_t i = 0; str[i] != '\0'; i++) {
        writer_write_char(self, str[i]);
    }
}

static void writer_clear_screen(Writer* self)
{
    for (size_t r = 0; r < VGA_HEIGHT; r++) {
        for (size_t col = 0; col < VGA_WIDTH; col++) {
            size_t index = r * VGA_WIDTH + col;
            self->buffer[index] = VGA_ENTRY(' ', self->colour);
        }
    }
    self->column = 0;
    self->row = 0;
}

static void writer_clear_char(Writer* self)
{
    if (self->column > 0) {
        self->column -= 1;
        size_t index = self->row * VGA_WIDTH + self->column;
        self->buffer[index] = VGA_ENTRY(' ', self->colour);
    }
}

static void writer_init(Writer* self)
{
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            size_t index = y * VGA_WIDTH + x;
            self->buffer[index] = VGA_ENTRY(' ', self->colour);
        }
    }
}

/* Public */

void vga_init(void) { writer_init(&WRITER); }

void vga_putchar(u8 c) { writer_write_char(&WRITER, c); }

void vga_puts(char const* str) { writer_write_string(&WRITER, str); }

void vga_clear(void) { writer_clear_screen(&WRITER); }

void vga_delete(void) { writer_clear_char(&WRITER); }
