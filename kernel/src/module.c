#include <ferrite/elf.h>
#include <ferrite/ksyms.h>
#include <ferrite/module.h>
#include <ferrite/string.h>
#include <uapi/errno.h>

#include "drivers/printk.h"
#include "memory/kmalloc.h"
#include <uapi/fcntl.h>

module_t* modules = NULL;

/* Private */

static int verify_elf_header(elf32_hdr_t* hdr)
{
    if (hdr->e_ident[EI_MAG0] != ELFMAG0 || hdr->e_ident[EI_MAG1] != ELFMAG1
        || hdr->e_ident[EI_MAG2] != ELFMAG2
        || hdr->e_ident[EI_MAG3] != ELFMAG3) {
        printk("Not a valid ELF file\n");
        return -EINVAL;
    }

    if (hdr->e_ident[EI_CLASS] != ELFCLASS32) {
        printk("Not a 32-bit ELF file\n");
        return -EINVAL;
    }

    if (hdr->e_ident[EI_DATA] != ELFDATA2LSB) {
        printk("Wrong endianness (expected little-endian)\n");
        return -EINVAL;
    }

    if (hdr->e_ident[EI_VERSION] != EV_CURRENT) {
        printk("Unsupported ELF version\n");
        return -EINVAL;
    }

    return 0;
}

static module_t* module_find(char const* name)
{
    if (!modules) {
        printk("No modules loaded\n");
        return NULL;
    }

    module_t* tmp = modules;

    while (tmp) {
        if (strcmp(tmp->name, name) == 0) {
            return tmp;
        }

        tmp = tmp->next;
    }

    return NULL;
}

static module_t* module_create(char const* name, void* code, size_t size)
{
    if (module_find(name)) {
        printk("Module '%s' already loaded\n", name);
        return NULL;
    }

    module_t* mod = kmalloc(sizeof(module_t));
    if (!mod) {
        return NULL;
    }

    memset(mod, 0, sizeof(module_t));

    memcpy(mod->name, name, MOD_MAX_NAME);
    mod->name[MOD_MAX_NAME - 1] = '\0';

    mod->code_addr = code;
    mod->code_size = size;

    mod->state = MODULE_STATE_LOADED;
    mod->ref_count = 0;

    mod->next = modules;
    modules = mod;

    printk(
        "Registered module '%s' at 0x%x (%u bytes)\n", mod->name,
        (u32)mod->code_addr, mod->code_size
    );

    return mod;
}

static int module_remove(char const* name)
{
    module_t** prev = &modules;
    module_t* mod = modules;

    while (mod) {
        if (strcmp(mod->name, name) == 0) {

            if (mod->ref_count > 0) {
                printk(
                    "Module '%s' is in use (refcount=%d)\n", name,
                    mod->ref_count
                );
                return -EBUSY;
            }

            if (mod->cleanup) {
                mod->cleanup();
            }

            *prev = mod->next;

            if (mod->code_addr) {
                kfree(mod->code_addr);
            }

            kfree(mod);

            printk("Module '%s' removed\n", name);
            return 0;
        }

        prev = &mod->next;
        mod = mod->next;
    }

    return -ENOENT;
}

/* Public */

void module_list(void)
{
    if (!modules) {
        printk("No modules loaded\n");
        return;
    }

    printk("Loaded modules:\n");
    printk("%s  %s  %s  %s\n", "Name", "Address", "Size", "State");
    printk("------------------------------------------------------------\n");

    module_t* tmp = modules;
    while (tmp) {
        char* state_str;
        switch (tmp->state) {
        case MODULE_STATE_LOADING:
            state_str = "loading";
            break;
        case MODULE_STATE_LOADED:
            state_str = "loaded";
            break;
        case MODULE_STATE_UNLOADING:
            state_str = "unloading";
            break;
        default:
            state_str = "unknown";
            break;
        }

        printk(
            "%s  0x%x  %u  %s\n", tmp->name, (u32)tmp->code_addr,
            tmp->code_size, state_str
        );

        tmp = tmp->next;
    }
}

int sys_delete_module(char const* name, unsigned int flags)
{
    if (!name) {
        printk("sys_delete_module: NULL module name\n");
        return -EINVAL;
    }

    if (strlen(name) >= MOD_MAX_NAME) {
        printk("sys_delete_module: module name too long\n");
        return -ENAMETOOLONG;
    }

    int force = (flags & O_TRUNC) != 0;
    if (force) {
        module_t* mod = module_find(name);
        if (!mod) {
            return -ENOENT;
        }

        if (mod->ref_count > 0) {
            printk(
                "WARNING: Force unloading module '%s' with refcount %d\n", name,
                mod->ref_count
            );
        }

        mod->ref_count = 0;
    }

    return module_remove(name);
}

int sys_init_module(void* mod, unsigned long len, char const* const args)
{
    (void)len;

    int error = 0;

    elf32_hdr_t* ehdr = (elf32_hdr_t*)mod;
    error = verify_elf_header(ehdr);
    if (error < 0) {
        return error;
    }

    elf32_shdr_t* sections = (elf32_shdr_t*)((char*)mod + ehdr->e_shoff);
    elf32_shdr_t* shstrtab_hdr = &sections[ehdr->e_shstrndx];
    char* shstrtab = (char*)mod + shstrtab_hdr->sh_offset;

    elf32_shdr_t* symtab_hdr = NULL;
    elf32_shdr_t* strtab_hdr = NULL;
    elf32_shdr_t* text_hdr = NULL;
    elf32_shdr_t* rodata_hdr = NULL;

    for (int i = 0; i < ehdr->e_shnum; i++) {
        char* name = shstrtab + sections[i].sh_name;

        if (strcmp(name, ".symtab") == 0) {
            symtab_hdr = &sections[i];
        } else if (strcmp(name, ".strtab") == 0) {
            strtab_hdr = &sections[i];
        } else if (strcmp(name, ".text") == 0) {
            text_hdr = &sections[i];
        } else if (strncmp(name, ".rodata", 7) == 0) {
            rodata_hdr = &sections[i];
        }
    }

    if (!symtab_hdr || !strtab_hdr || !text_hdr) {
        printk("Missing required sections\n");
        return -EINVAL;
    }

    unsigned long total_size = 0;
    unsigned long text_offset = 0;
    unsigned long rodata_offset = 0;

    text_offset = total_size;
    total_size += text_hdr->sh_size;

    if (rodata_hdr) {
        rodata_offset = total_size;
        total_size += rodata_hdr->sh_size;
    }

    void* mod_mem = kmalloc(total_size);
    if (!mod_mem) {
        return -ENOMEM;
    }

    memcpy(
        (char*)mod_mem + text_offset, (char*)mod + text_hdr->sh_offset,
        text_hdr->sh_size
    );

    if (rodata_hdr) {
        memcpy(
            (char*)mod_mem + rodata_offset, (char*)mod + rodata_hdr->sh_offset,
            rodata_hdr->sh_size
        );
    }

    unsigned long section_addrs[256] = { 0 };
    for (int i = 0; i < ehdr->e_shnum && i < 256; i++) {
        char* name = shstrtab + sections[i].sh_name;

        if (strcmp(name, ".text") == 0) {
            section_addrs[i] = (unsigned long)mod_mem + text_offset;
        } else if (strncmp(name, ".rodata", 7) == 0) {
            section_addrs[i] = (unsigned long)mod_mem + rodata_offset;
        }
    }

    Elf32_Sym* symtab = (Elf32_Sym*)((char*)mod + symtab_hdr->sh_offset);
    char* strtab = (char*)mod + strtab_hdr->sh_offset;

    for (int i = 0; i < ehdr->e_shnum; i++) {
        if (sections[i].sh_type != SHT_REL) {
            continue;
        }

        char* name = shstrtab + sections[i].sh_name;
        if (strcmp(name, ".rel.text") != 0) {
            continue;
        }

        Elf32_Rel* rels = (Elf32_Rel*)((char*)mod + sections[i].sh_offset);
        unsigned long num_rels = sections[i].sh_size / sizeof(Elf32_Rel);

        for (unsigned long j = 0; j < num_rels; j++) {
            unsigned long* location
                = (unsigned long*)((char*)mod_mem + text_offset
                                   + rels[j].r_offset);
            int sym_idx = ELF32_R_SYM(rels[j].r_info);
            int rel_type = ELF32_R_TYPE(rels[j].r_info);

            Elf32_Sym* sym = &symtab[sym_idx];
            unsigned long sym_addr = 0;

            if (ELF32_ST_TYPE(sym->st_info) == STT_SECTION) {
                sym_addr = section_addrs[sym->st_shndx];
            } else if (sym->st_shndx == 0) {
                char* sym_name = strtab + sym->st_name;
                sym_addr = ksym_lookup(sym_name);
                if (sym_addr == 0) {
                    printk("Undefined symbol: %s\n", sym_name);
                    kfree(mod_mem);
                    return -EINVAL;
                }
            } else {
                sym_addr = section_addrs[sym->st_shndx] + sym->st_value;
            }

            switch (rel_type) {
            case R_386_32:
                *location += sym_addr;
                break;

            case R_386_PC32: {
                unsigned long P
                    = (unsigned long)mod_mem + text_offset + rels[j].r_offset;
                *location += sym_addr - P;
                break;
            }

            default:
                printk("Unknown relocation type: %d\n", rel_type);
                kfree(mod_mem);
                return -EINVAL;
            }
        }
    }

    int (*init_fn)(void) = NULL;
    unsigned long num_syms = symtab_hdr->sh_size / sizeof(Elf32_Sym);

    for (unsigned long i = 0; i < num_syms; i++) {
        char* name = strtab + symtab[i].st_name;

        if (strcmp(name, "init_module") == 0) {
            void* addr = (char*)mod_mem + text_offset + symtab[i].st_value;
            memcpy((void*)&init_fn, (void const*)&addr, sizeof(init_fn));
            break;
        }
    }

    if (!init_fn) {
        printk("init_module function not found\n");
        kfree(mod_mem);
        return -EINVAL;
    }

    int ret = init_fn();
    if (ret < 0) {
        printk("init_module failed with error %d\n", ret);
        kfree(mod_mem);
        return ret;
    }

    char const* module_name = !args ? "unknown" : args;
    module_t* module = module_create(module_name, mod_mem, total_size);
    if (!module) {
        printk("Failed to register module\n");
        kfree(mod_mem);
        return -ENOMEM;
    }

    module->init = init_fn;
    for (unsigned long i = 0; i < num_syms; i++) {
        char* name = strtab + symtab[i].st_name;
        if (strcmp(name, "cleanup_module") == 0) {
            void* addr = (char*)mod_mem + text_offset + symtab[i].st_value;
            memcpy(
                (void*)&module->cleanup, (void const*)&addr,
                sizeof(module->cleanup)
            );
            break;
        }
    }

    return 0;
}
