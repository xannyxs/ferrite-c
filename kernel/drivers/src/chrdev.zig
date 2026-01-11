const c = @cImport({
    @cInclude("ferrite/errno.h");
    @cInclude("ferrite/types.h");
    @cInclude("drivers/chrdev.h");
});

const ChrdevOps = c.chrdev_ops_t;

const MAX_CHRDEV = 256;
var chrdev_table: [MAX_CHRDEV]?*const ChrdevOps = [_]?*const ChrdevOps{null} ** MAX_CHRDEV;

pub export fn register_chrdev(major: c_uint, ops: *const ChrdevOps) c_int {
    if (major >= MAX_CHRDEV) {
        return -c.EINVAL;
    }

    if (chrdev_table[major] != null) {
        return -c.EBUSY;
    }

    chrdev_table[major] = ops;
    return 0;
}

pub export fn get_chrdev(major: c_uint) ?*const ChrdevOps {
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
