const c = @cImport({
    @cInclude("ferrite/errno.h");
    @cInclude("ferrite/types.h");
    @cInclude("drivers/chrdev.h");
    @cInclude("sys/file/file.h");
    @cInclude("drivers/printk.h");
});

const MAX_CHRDEV = 32;
var chrdev_table: [MAX_CHRDEV]?*const c.struct_file_operations = [_]?*const c.struct_file_operations{null} ** MAX_CHRDEV;

pub export fn register_chrdev(major: c_uint, ops: *const c.struct_file_operations) c_int {
    if (major >= MAX_CHRDEV) {
        return -c.EINVAL;
    }

    if (chrdev_table[major] != null) {
        return -c.EBUSY;
    }

    chrdev_table[major] = ops;
    return 0;
}

pub export fn get_chrdev(major: c_uint) ?*const c.struct_file_operations {
    if (major >= MAX_CHRDEV) {
        return null;
    }

    return chrdev_table[major];
}

pub export fn unregister_chrdev(major: c_uint) c_int {
    if (major >= MAX_CHRDEV) {
        return -c.EINVAL;
    }

    chrdev_table[major] = null;
    return 0;
}
