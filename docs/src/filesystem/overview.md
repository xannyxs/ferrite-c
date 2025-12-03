# Filesystem overview

This chapter covers the filesystem architecture. The kernel currently supports ext2 with the following status:

**Current state:**

- ✓ Basic file operations (create, read, write, delete)
- ✓ Directory operations (mkdir, rmdir, chdir, getcwd)
- ✓ Path resolution (absolute and relative paths)
- ✗ Symbolic links
- ✗ Hard links
- ✗ Multiple filesystem support

We will explore three key layers:

1. **Block Devices** - Hardware abstraction for disk access
2. **VFS Layer** - Virtual filesystem providing a unified interface
3. **ext2 Implementation** - Filesystem driver and on-disk format

Each section covers design decisions & implementation details of my kernel.
