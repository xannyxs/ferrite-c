#include "drivers/block/ide.h"
#include "arch/x86/io.h"
#include "drivers/block/device.h"
#include "drivers/printk.h"
#include "ferrite/major.h"
#include "memory/kmalloc.h"

#include <ferrite/types.h>
#include <lib/string.h>

#define ATA_ADDR 0x1F0
#define DEVICE_ATAPI 1
#define DEVICE_ATA 2

/* Private */

u16 ata_data[256];

static void parse_vender_name(ata_drive_t* s)
{
    for (int i = 0; i < 20; i++) {
        s->name[i * 2] = (char)((ata_data[27 + i] >> 8) & 0xFF);
        s->name[(i * 2) + 1] = (char)(ata_data[27 + i] & 0xFF);
    }
    s->name[40] = '\0';

    for (int i = 39; i >= 0 && s->name[i] == ' '; i--) {
        s->name[i] = '\0';
    }

    printk("name: %s\n", s->name);
}

static int device_type(u16 device)
{
    if (device & 0x8000) {
        printk("This is an ATAPI device (CD-ROM, etc)\n");
        return DEVICE_ATAPI;
    }

    if (device == 0x0000 || (device & 0x0040)) {
        printk("This is an ATA hard disk\n");
        return DEVICE_ATA;
    }

    return -1;
}

static ata_drive_t* detect_harddrives(u8 master)
{
    u8 select = (master ? 0xb0 : 0xa0);
    outb(ATA_ADDR + 6, select);

    outb(ATA_ADDR + 2, 0);
    outb(ATA_ADDR + 3, 0);
    outb(ATA_ADDR + 4, 0);
    outb(ATA_ADDR + 5, 0);

    outb(ATA_ADDR + 7, 0xEC);

    u8 status = inb(ATA_ADDR + 7);
    if (status == 0) {
        printk("No ATA exists on %s!\n", (select ? "slave" : "master"));
        return NULL;
    }

    while (status & 0x80) {
        status = inb(ATA_ADDR + 7);
    }

    if (status & 0x01) {
        return NULL;
    }

    if (inb(ATA_ADDR + 4) != 0 || inb(ATA_ADDR + 5) != 0) {
        return NULL; // Not a ATA
    }

    int timeout = 1000;
    while ((status & 0x08) == 0 && timeout > 0) {
        status = inb(ATA_ADDR + 7);
        timeout--;
    }

    if (timeout == 0) {
        printk("Timeout waiting for DRQ\n");
        return NULL;
    }

    for (int i = 0; i < 256; i += 1) {
        ata_data[i] = inw(ATA_ADDR + 0);
    }

    if (device_type(ata_data[0]) != DEVICE_ATA) {
        return NULL;
    }

    ata_drive_t* ata_drive = kmalloc(sizeof(ata_drive_t));
    if (!ata_drive) {
        return NULL;
    }

    parse_vender_name(ata_drive);
    ata_drive->lba28_sectors = ata_data[60] | ((u32)ata_data[61] << 16);
    ata_drive->drive = master;

    if (ata_data[83] & (1 << 10)) {
        printk("supports lba48 mode. Ignore for now...\n");

        ata_drive->lba48_sectors = (unsigned long long)ata_data[100]
            | ((unsigned long long)ata_data[101] << 16)
            | ((unsigned long long)ata_data[102] << 32)
            | ((unsigned long long)ata_data[103] << 48);
        ata_drive->supports_lba48 = 1;
    } else {
        ata_drive->lba48_sectors = 0;
        ata_drive->supports_lba48 = 0;
    }

    return ata_drive;
}

/* Public */

u32 read_from_ata_data(void)
{
    u16 word = ata_data[106];

    if ((word & 0xC000) != 0x4000) {
        printk("Word 106 not valid, assuming 512-byte sectors\n");
        return 512;
    }

    if (!(word & (1 << 12))) {
        printk("Logical sector size not reported, assuming 512 bytes\n");
        return 512;
    }

    u32 sector_size_words = ata_data[117] | ((u32)ata_data[118] << 16);
    if (sector_size_words == 0) {
        printk("Sector size is 0, assuming 512 bytes\n");
        return 512;
    }

    return sector_size_words * 2;
}

static s32
ide_read(block_device_t* d, u32 lba, u32 count, void* buf, size_t len);
static s32
ide_write(block_device_t* d, u32 lba, u32 count, void const* buf, size_t len);
static void ide_shutdown(block_device_t* d);

struct device_operations ide_device_ops = {
    .read = ide_read,
    .write = ide_write,
    .shutdown = ide_shutdown,
};

s32 ide_read(block_device_t* d, u32 lba, u32 count, void* buf, size_t len)
{
    ata_drive_t* ata = (ata_drive_t*)d->d_data;

    if (len < count * d->d_sector_size) {
        printk("Buffer too small\n");
        return -1;
    }

    outb(0x1F6, 0xE0 | (ata->drive << 4) | ((lba >> 24) & 0x0F));
    outb(0x1F1, 0x00);

    inb(0x3F6);
    inb(0x3F6);
    inb(0x3F6);
    inb(0x3F6);

    outb(0x1F2, (u8)count);
    outb(0x1F3, (u8)lba);
    outb(0x1F4, (u8)(lba >> 8));
    outb(0x1F5, (u8)(lba >> 16));
    outb(0x1F7, 0x20);

    inb(0x3F6);
    inb(0x3F6);
    inb(0x3F6);
    inb(0x3F6);

    u16* buffer = (u16*)buf;

    for (u32 sector = 0; sector < count; sector += 1) {
        u8 status = inb(ATA_ADDR + 7);
        while (status & 0x80) {
            status = inb(ATA_ADDR + 7);
        }

        if (status & 0x01) {
            printk("Read error\n");
            return -1;
        }

        while (!(status & 0x08)) {
            status = inb(ATA_ADDR + 7);
        }

        for (int i = 0; i < 256; i++) {
            buffer[sector * 256 + i] = inw(ATA_ADDR + 0);
        }
    }

    return 0;
}

s32 ide_write(
    block_device_t* d,
    u32 lba,
    u32 count,
    void const* buf,
    size_t len
)
{
    if (!d->d_data) {
        printk("d_data is NULL\n", d->d_data);
        return -1;
    }
    if (len < count * d->d_sector_size) {
        printk("Buffer too small\n");
        return -1;
    }

    ata_drive_t* ata = (ata_drive_t*)d->d_data;

    outb(0x1F6, 0xE0 | (ata->drive << 4) | ((lba >> 24) & 0x0F));
    outb(0x1F1, 0x00);

    outb(0x1F2, (u8)count);
    outb(0x1F3, (u8)lba);
    outb(0x1F4, (u8)(lba >> 8));
    outb(0x1F5, (u8)(lba >> 16));
    outb(0x1F7, 0x30);

    u16 const* buffer = (u16 const*)buf;
    for (u32 sector = 0; sector < count; sector += 1) {
        u8 status = inb(ATA_ADDR + 7);
        while (status & 0x80) {
            status = inb(ATA_ADDR + 7);
        }

        if (status & 0x01) {
            printk("Write error\n");
            return -1;
        }

        int timeout = 1000;
        status = inb(0x1F7);
        while ((status & 0x08) == 0 && timeout > 0) {
            status = inb(ATA_ADDR + 7);
            timeout--;
        }

        if (timeout == 0) {
            printk("Timeout waiting for DRQ\n");
            return -1;
        }

        for (int i = 0; i < 256; i += 1) {
            outw(ATA_ADDR + 0, buffer[sector * 256 + i]);
        }
    }
    outb(0x1F7, 0xE7);

    return 0;
}

// TODO
void ide_shutdown(block_device_t* d) { (void)d; }

// FIXME: Minor is for particions. Should we support it?
s32 ide_mount(s32 major, s32 minor)
{
    (void)minor;

    if (major == IDE0_MAJOR) {
        ata_drive_t* d = detect_harddrives(0);
        if (!d) {
            return -1;
        }

        register_block_device(BLOCK_DEVICE_IDE, d);
        return 0;
    }

    if (major == IDE1_MAJOR) {
        ata_drive_t* d = detect_harddrives(1);
        if (!d) {
            return -1;
        }

        register_block_device(BLOCK_DEVICE_IDE, d);
    }

    return 0;
}
