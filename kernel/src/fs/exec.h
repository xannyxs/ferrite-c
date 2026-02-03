#ifndef _BIN_PROGRAMS_H
#define _BIN_PROGRAMS_H

#include "arch/x86/idt/idt.h"
#include "fs/vfs.h"

/*
 * From Linux 1.0:
 * https://elixir.bootlin.com/linux/1.0.9/source/include/linux/binfmts.h#L16
 *
 * MAX_ARG_PAGES defines the number of pages allocated for arguments
 * and envelope for the new program. 32 should suffice, this gives
 * a maximum env+arg of 128kB !
 */
#define MAX_ARG_PAGES 32

/*
 * This structure is used to hold the arguments that are used when loading
 * binaries.
 */
typedef struct binpgm {
    char b_buf[128];

    unsigned long b_page[MAX_ARG_PAGES];
    unsigned long b_p;

    vfs_inode_t* b_node;

    int e_uid, e_gid;
    int argc, envc;

    int b_sh_bang;
    char* b_filename;
} binpgm_t;

typedef struct binfmt {
    int (*load_binary)(binpgm_t*, trapframe_t*);
    int (*load_shlib)(int);
} binfmt_t;

/* exec.c */
int read_exec(vfs_inode_t*, int, char*, int);

int do_execve(char const*, char const* const*, char const* const*, trapframe_t*);

/* binfmt_elf.c */
int load_elf_binary(binpgm_t*, trapframe_t*);

#endif
