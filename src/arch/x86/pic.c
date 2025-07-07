#include "arch/x86/pic.h"
#include "arch/x86/io.h"

void pic_remap(int32_t offset1, int32_t offset2) {
  // starts the initialization sequence (in cascade mode)
  outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
  io_wait();
  outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
  io_wait();

  outb(PIC1_DATA, offset1); // ICW2: Master PIC vector offset
  io_wait();
  outb(PIC2_DATA, offset2); // ICW2: Slave PIC vector offset
  io_wait();

  // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
  outb(PIC1_DATA, 4);
  io_wait();

  // ICW3: tell Slave PIC its cascade identity (0000 0010)
  outb(PIC2_DATA, 2);
  io_wait();

  // ICW4: have the PICs use 8086 mode (and not 8080 mode)
  outb(PIC1_DATA, ICW4_8086);
  io_wait();
  outb(PIC2_DATA, ICW4_8086);
  io_wait();

  // Unmask both PICs.
  outb(PIC2_DATA, 0xFE);
  outb(PIC1_DATA, 0xF9);
}
