#include "drivers/block/ide.h"
#include "arch/x86/io.h"
#include "drivers/block/device.h"
#include "drivers/printk.h"
#include "ferrite/major.h"
#include "memory/kmalloc.h"

#include <ferrite/errno.h>
#include <ferrite/string.h>
#include <ferrite/types.h>

#define DEVICE_ATAPI 1
#define DEVICE_ATA 2

static ide_controller_t ide_controllers[MAX_IDE_CONTR]
    = { { .base_port = 0x1F0, .ctrl_port = 0x3F6, .irq = 14 },
        { .base_port = 0x170, .ctrl_port = 0x376, .irq = 15 } };

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

/*
 * Warning: Returned value is dynamicly allocated
 */
static ata_drive_t* detect_harddrives(u8 controller_num, u8 master)
{
    if (controller_num >= 2 || !ide_controllers[controller_num].present) {
        printk("%s: Controller %d not present\n", __func__, controller_num);
        return NULL;
    }

    u16 base_port = ide_controllers[controller_num].base_port;

    u8 select = (master ? 0xb0 : 0xa0);
    outb(base_port + 6, select);

    outb(base_port + 2, 0);
    outb(base_port + 3, 0);
    outb(base_port + 4, 0);
    outb(base_port + 5, 0);

    outb(base_port + 7, 0xEC);

    u8 status = inb(base_port + 7);
    if (status == 0) {
        printk(
            "%s: No drive on controller %d, drive %d\n", __func__,
            controller_num, master
        );
        return NULL;
    }

    while (status & 0x80) {
        status = inb(base_port + 7);
    }

    if (status & 0x01) {
        printk(
            "%s: Error bit set on controller %d, drive %d\n", __func__,
            controller_num, master
        );
        return NULL;
    }

    if (inb(base_port + 4) != 0 || inb(base_port + 5) != 0) {
        printk(
            "%s: Not an ATA device on controller %d, drive %d\n", __func__,
            controller_num, master
        );
        return NULL;
    }

    int timeout = 1000;
    while ((status & 0x08) == 0 && timeout > 0) {
        status = inb(base_port + 7);
        timeout--;
    }

    if (timeout == 0) {
        printk(
            "%s: Timeout waiting for DRQ on controller %d, drive %d\n",
            __func__, controller_num, master
        );
        return NULL;
    }

    for (int i = 0; i < 256; i += 1) {
        ata_data[i] = inw(base_port + 0);
    }

    if (device_type(ata_data[0]) != DEVICE_ATA) {
        printk(
            "%s: Device type check failed on controller %d, drive %d\n",
            __func__, controller_num, master
        );
        return NULL;
    }

    ata_drive_t* ata_drive = kmalloc(sizeof(ata_drive_t));
    if (!ata_drive) {
        printk("%s: Failed to allocate memory for drive structure\n", __func__);
        return NULL;
    }
    parse_vender_name(ata_drive);
    ata_drive->lba28_sectors = ata_data[60] | ((u32)ata_data[61] << 16);
    ata_drive->drive = master;

    if (ata_data[83] & (1 << 10)) {
        ata_drive->lba48_sectors = (unsigned long long)ata_data[100]
            | ((unsigned long long)ata_data[101] << 16)
            | ((unsigned long long)ata_data[102] << 32)
            | ((unsigned long long)ata_data[103] << 48);
        ata_drive->supports_lba48 = 1;

        printk(
            "IDE: Drive supports LBA48 (%llu sectors)\n",
            ata_drive->lba48_sectors
        );
    } else {
        ata_drive->lba48_sectors = 0;
        ata_drive->supports_lba48 = 0;
    }

    printk(
        "IDE: Detected %s on controller %d, drive %d (%u sectors)\n",
        ata_drive->name, controller_num, master, ata_drive->lba28_sectors
    );

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

#define ATA_ADDR 0x1F0

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

    inb(0x3F6);
    inb(0x3F6);
    inb(0x3F6);
    inb(0x3F6);

    outb(0x1F2, (u8)count);
    outb(0x1F3, (u8)lba);
    outb(0x1F4, (u8)(lba >> 8));
    outb(0x1F5, (u8)(lba >> 16));
    outb(0x1F7, 0x30);

    inb(0x3F6);
    inb(0x3F6);
    inb(0x3F6);
    inb(0x3F6);

    u16 const* buffer = (u16 const*)buf;
    for (u32 sector = 0; sector < count; sector += 1) {
        u8 status;
        u32 timeout;

        timeout = 1000000;
        do {
            status = inb(0x1F7);
            if (--timeout == 0) {
                printk(
                    "ide_write: Timeout waiting for BSY clear (status=0x%x)\n",
                    status
                );
                return -1;
            }
        } while (status & 0x80);

        if (status & 0x01) {
            u8 error = inb(0x1F1);
            printk("%s: Error 0x%x\n", __func__, error);
            return -1;
        }

        timeout = 1000000;
        do {
            status = inb(0x1F7);
            if (--timeout == 0) {
                printk(
                    "ide_write: Timeout waiting for DRQ (status=0x%x)\n", status
                );
                return -1;
            }
        } while (!(status & 0x08));

        for (int i = 0; i < 256; i += 1) {
            outw(ATA_ADDR + 0, buffer[sector * 256 + i]);
        }
    }

    u32 timeout = 1000000;
    u8 status;
    do {
        status = inb(0x1F7);
        if (--timeout == 0) {
            printk("ide_write: Timeout waiting for write completion\n");
            return -1;
        }
    } while (status & 0x80);

    outb(0x1F7, 0xE7);
    inb(0x3F6);
    inb(0x3F6);
    inb(0x3F6);
    inb(0x3F6);

    timeout = 1000000;
    do {
        status = inb(0x1F7);
        if (--timeout == 0) {
            printk("ide_write: Timeout waiting for FLUSH\n");
            return -1;
        }
    } while (status & 0x80);

    return 0;
}

// TODO
void ide_shutdown(block_device_t* d) { (void)d; }

int ide_detach(dev_t bdev)
{
    block_device_t* d = get_device(bdev);
    if (!d) {
        return -ENODEV;
    }

    if (d->d_data) {
        kfree(d->d_data);
        d->d_data = NULL;
    }

    d->d_dev = 0;
    d->d_op = NULL;
    d->d_type = 0;

    printk("IDE: Detached device 0x%x\n", bdev);
    return 0;
}

int ide_probe(dev_t bdev)
{
    int major = MAJOR(bdev);
    int minor = MINOR(bdev);

    u8 controller_num = (major == IDE0_MAJOR) ? 0 : 1;
    u8 drive_num = (minor >> 4) & 0x01;

    ata_drive_t* d = detect_harddrives(controller_num, drive_num);
    if (!d) {
        return -ENODEV;
    }

    register_block_device(bdev, BLOCK_DEVICE_IDE, d);
    printk("IDE: Probed and registered device 0x%x\n", bdev);

    return 0;
}

static int ide_detect_controller(u8 controller_num)
{
    u16 base = ide_controllers[controller_num].base_port;
    u8 status = inb(base + 7);

    if (status == 0xFF) {
        return 0;
    }

    return 1;
}

void ide_init(void)
{
    printk("IDE: Initializing controllers\n");

    ide_controllers[0].present = ide_detect_controller(0);
    ide_controllers[1].present = ide_detect_controller(1);

    printk(
        "IDE: IDE0 %s, IDE1 %s\n",
        ide_controllers[0].present ? "present" : "not found",
        ide_controllers[1].present ? "present" : "not found"
    );
}
