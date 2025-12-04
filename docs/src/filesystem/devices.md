# Block Devices

Block devices are physical storage devices connected to your CPU.
Several types exist including SATA, (P)ATA, NVMe, and SSDs. The kernel
currently only supports ATA, one of the older storage interfaces.

The first ATA interface standard was known as IDE (Integrated Drive Electronics).
IDE was notable because it had integrated drive controllers built-in,
which was unique at the time.

## Initialization

The first step is finding the root drive for the kernel to use.
We accomplish this with help from the Multiboot bootloader.
Multiboot provides a `cmdline` variable for passing additional flags to the kernel.
You can define these flags yourself and pass them in `grub.cfg`.
I chose `root=` as my flag to specify the root device:

```
multiboot /boot/ferrite-c.elf root=/dev/hda1
```

This tells GRUB to execute multiboot with the kernel ELF file
and passes the `cmdline` with the `root=` flag.

**Note:** We reference `/dev/hda1` even though we haven't set up a `/dev`
directory yet. This is just a naming convention. We only parse the device
identifier from the path.

## Device Naming Convention

To extract the device name, parse everything after the last `/`,
resulting in `hda1`. This follows the Linux device naming scheme:

- **First two characters**: Device type (`hd` = IDE/ATA)
- **Third character**: Device number (`a` = first, `b` = second, etc.) also called the "major" number
- **Fourth character**: Partition number (the "minor" number)

For example, `hda1` means: IDE device A, partition 1.

To learn more about this, run `lsblk` in a Unix-like terminal to see how your computer sees your drives.
[This link](https://tldp.org/HOWTO/html_single/Partition/#names) gives your more details on the Linux naming convention.

## Parsing the Root Device

Here's the implementation:

```c
void root_device_init(char* cmdline)
{
    char* root_device_line = strnstr(cmdline, "root=", 32);
    if (!root_device_line) {
        abort("No root= parameter in cmdline");
    }
    
    char* device = strrchr(root_device_line, '/');
    if (!device) {
        abort("Invalid root device format");
    }
    
    char* type_device = strnstr(device, "hd", 12);
    if (!type_device || strncmp(type_device, "hd", 2) != 0) {
        abort("No IDE device specified");
    }

    // Parse major (controller) from device letter
    // 'a','b' -> IDE0, 'c','d' -> IDE1
    s32 index = type_device[2] - 'a'; // 0=a, 1=b, 2=c, 3=d
    s32 major = (index < 2) ? IDE0_MAJOR : IDE1_MAJOR;

    s32 minor = type_device[3] - '0';

    if (ide_mount(major, minor) < 0) {
        abort("Failed to mount root device");
    }
}
```

## IDE Architecture

IDE supports up to 4 drives on two controllers:

- **Primary IDE (IDE0_MAJOR)**: `hda` (master), `hdb` (slave)
- **Secondary IDE (IDE1_MAJOR)**: `hdc` (master), `hdd` (slave)

You might wonder why `IDE0_MAJOR` isn't 0 and `IDE1_MAJOR` isn't 1.
This comes from Linux 1.0's device numbering scheme. IDE major numbers were
assigned specific values (3 for IDE0, 22 for IDE1) to maintain compatibility
with existing device numbers.

**Note:** The minor number (partition) is not yet used in the kernel
and will be implemented in the future to support multiple partitions per drive.

## Mounting the Device

```c
s32 ide_mount(s32 major, s32 minor)
{
    if (major == IDE0_MAJOR) {
        ata_drive_t* d = detect_harddrives(0);
        register_block_device(BLOCK_DEVICE_IDE, d);
    } else if (major == IDE1_MAJOR) {
        ata_drive_t* d = detect_harddrives(1);
        register_block_device(BLOCK_DEVICE_IDE, d);
    }

    return 0;
}
```

We detect if the hard drives exist and register them as available block devices.
Drive detection follows specific hardware protocols.
Check out [OSDev ATA PIO Mode](https://wiki.osdev.org/ATA_PIO_Mode) for more details.

## The block_device Structure

Each storage device requires different instructions for reading and writing.
To handle this, we use function pointers that point to device-specific implementations.
For IDE devices, the read function points to `ide_read()`, allowing correct
operations without runtime type checking.

```c
typedef struct {
    dev_t d_dev;              // Device identifier
    u32 d_sector_size;        // Sector size in bytes
    
    void* d_data;             // Device-specific data
    
    block_device_type_e d_type;
    struct device_operations* d_op;
} block_device_t;

struct device_operations {
    s32 (*read)(block_device_t*, u32 lba, u32 count, void*, size_t len);
    s32 (*write)(block_device_t*, u32 lba, u32 count, void const*, size_t len);
};
```

### Design Philosophy

The `block_device` structure maintains minimal interaction with the VFS layer
to ensure separation of concerns. The `dev_t` identifier allows the VFS
superblock to know which physical device to target without understanding device-specific details.

This abstraction layer enables the VFS to interact with any block device uniformly,
whether it's IDE, SATA, or future device types, without knowing implementation details.
