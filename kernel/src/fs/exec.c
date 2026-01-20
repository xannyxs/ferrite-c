#include "fs/exec.h"
#include "drivers/printk.h"
#include "ferrite/types.h"
#include <uapi/stat.h>
#include "fs/vfs.h"
#include "idt/idt.h"
#include "memory/page.h"
#include "sys/file/file.h"
#include "sys/process/process.h"

#include <uapi/errno.h>
#include <ferrite/string.h>
#include <lib/stdlib.h>
#include <memory/consts.h>
#include <sys/file/fcntl.h>

/* Formats */

/* NOTE: Kernel only supports ELF (for now), not a.out, since it is obsolete */

binfmt_t supported_formats[] = { { load_elf_binary, NULL }, { NULL, NULL } };

/* Private */

static int count(char const* const* array)
{
    int i = 0;
    if (!array) {
        return 0;
    }

    while (array[i]) {
        i++;
    }

    return i;
}

static unsigned long copy_strings(
    int argc,
    char const* const argv[],
    unsigned long* page,
    unsigned long p
)
{

    for (int i = argc - 1; i >= 0; i -= 1) {
        char const* str = (char*)argv[i];
        size_t len = strlen(str) + 1;

        if (p < len) {
            return 0;
        }

        p -= len;

        for (size_t j = 0; j < len; j += 1) {
            int page_index = (p + j) / PAGE_SIZE;
            int offset = (p + j) % PAGE_SIZE;

            if (!page[page_index]) {
                page[page_index] = (unsigned long)get_free_page();
                if (!page[page_index]) {
                    return 0;
                }
            }

            ((char*)page[page_index])[offset] = str[j];
        }
    }

    return p;
}

int read_exec(vfs_inode_t* node, int offset, char* addr, int count)
{
    if (count > PAGE_SIZE * 16 || (u32)offset > node->i_size) {
        return -EINVAL;
    }

    file_t file;
    int result = -ENOEXEC;

    if (!node->i_op || !node->i_op->default_file_ops) {
        goto end_readexec;
    }

    file.f_mode = FMODE_READ;
    file.f_flags = 0;
    file.f_count = 1;
    file.f_inode = node;
    file.f_pos = 0;
    file.f_op = node->i_op->default_file_ops;

    if (file.f_op->open) {
        if (file.f_op->open(node, &file))
            goto end_readexec;
    }

    if (!file.f_op || !file.f_op->read) {
        goto close_readexec;
    }

    if (file.f_op->lseek) {
        if (file.f_op->lseek(node, &file, offset, SEEK_SET) != offset) {
            goto close_readexec;
        }
    } else {
        file.f_pos = offset;
    }

    result = file.f_op->read(node, &file, addr, count);

close_readexec:
    if (file.f_op->release) {
        file.f_op->release(node, &file);
    }

end_readexec:
    return result;
}

/* Public */

int do_execve(
    char const* filename,
    char const* const* argv,
    char const* const* envp,
    trapframe_t* regs
)
{
    int retval = 0;

    binfmt_t* fmt = supported_formats;
    binpgm_t bin = { 0 };

    bin.b_p = PAGE_SIZE * MAX_ARG_PAGES - 4;
    for (int i = 0; i < MAX_ARG_PAGES; i += 1) {
        bin.b_page[i] = 0;
    }

    bin.b_node = vfs_lookup(myproc()->root, filename);
    if (!bin.b_node) {
        return -ENOENT;
    }

    bin.b_filename = (char*)filename;
    bin.argc = count(argv);
    bin.envc = count(envp);

    if (!S_ISREG(bin.b_node->i_mode)) {
        retval = -EACCES;
        goto exec_error;
    }
    if (!bin.b_node->i_sb) {
        retval = -EACCES;
        goto exec_error;
    }

    memset(bin.b_buf, 0, sizeof(bin.b_buf));
    retval = read_exec(bin.b_node, 0, bin.b_buf, 128);
    if (retval < 0) {
        goto exec_error;
    }

    bin.b_p = copy_strings(bin.envc, envp, bin.b_page, bin.b_p);
    if (bin.b_p == 0) {
        retval = -E2BIG;
        goto exec_error;
    }

    bin.b_p = copy_strings(bin.argc, argv, bin.b_page, bin.b_p);
    if (bin.b_p == 0) {
        retval = -E2BIG;
        goto exec_error;
    }

    do {
        if (!fmt->load_binary) {
            break;
        }

        retval = fmt->load_binary(&bin, regs);
        if (retval == 0) {
            inode_put(bin.b_node);

            return 0;
        }

        fmt++;
    } while (retval == -ENOEXEC);

exec_error:
    inode_put(bin.b_node);

    for (int i = 0; i < MAX_ARG_PAGES; i++) {
        if (bin.b_page[i]) {
            free_page((void*)bin.b_page[i]);
        }
    }

    return retval;
}
