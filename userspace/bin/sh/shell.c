#include <libc/stdio.h>
#include <libc/string.h>
#include <libc/syscalls.h>
#include <uapi/errno.h>

#define MAX_LINE 256
#define MAX_ARGS 32

/* Private */

static void print_help(void)
{
    printf("\n");
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║            Ferrite Kernel Shell - Help                ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n\n");

    printf("SYSTEM\n");
    printf("  reboot                      Restart the system\n");
    printf("  clear                       Clear the terminal screen\n");
    printf("  time                        Show current date and time\n");
    printf("  epoch                       Show Unix timestamp\n");
    printf("  sleep <milliseconds>        Pause for specified duration\n");
    printf("\n");

    printf("DIAGNOSTICS\n");
    printf("  gdt                         Dump Global Descriptor Table\n");
    printf("  idt                         Dump Interrupt Descriptor Table\n");
    printf("  memory                      Show memory allocator status\n");
    printf("  top                         List running processes\n");
    printf("  devices                     List block devices\n");
    printf("\n");

    printf("FILE OPERATIONS\n");
    printf("  ls [path]                   List directory (default: current)\n");
    printf("  cd <directory>              Change working directory\n");
    printf("  pwd                         Print working directory\n");
    printf("  cat <file>                  Display file contents\n");
    printf("  echo <text>                 Write text to file\n");
    printf("  touch <file>                Create empty file\n");
    printf("  rm <file>                   Delete file\n");
    printf("  mkdir <directory>           Create directory\n");
    printf("  rmdir <directory>           Remove empty directory\n");
    printf("\n");

    printf("FILESYSTEMS\n");
    printf("  mount <device> <path>       Mount filesystem\n");
    printf("                              Example: mount /dev/hdb /mnt\n");
    printf("  umount <device>             Unmount filesystem\n");
    printf("\n");

    printf("MODULES\n");
    printf("  insmod <module.o>           Load kernel module\n");
    printf("  rmmod <module_name>         Unload kernel module\n");
    printf("  lsmod                       List loaded modules\n");
    printf("\n");
}

static int builtin_cd(int argc, char** argv)
{
    if (argc < 2) {
        printf("cd: missing argument\n");
        return 1;
    }

    if (chdir(argv[1]) < 0) {
        printf("cd: cannot change directory to '%s'\n", argv[1]);
        return 1;
    }

    return 0;
}

static int builtin_pwd(void)
{
    char buf[512];

    if (getcwd(buf, sizeof(buf)) >= 0) {
        printf("%s\n", buf);
        return 0;
    }

    printf("pwd: failed\n");
    return -1;
}

static int parse(char* line, char** argv)
{
    int argc = 0;

    while (*line) {
        while (*line == ' ')
            line++;
        if (!*line)
            break;

        argv[argc++] = line;

        while (*line && *line != ' ')
            line++;
        if (*line)
            *line++ = '\0';
    }

    argv[argc] = 0;
    return argc;
}

/* Public */

int main(void)
{
    char line[MAX_LINE];
    char* argv[MAX_ARGS];

    printf("Ferrite Shell v0.1\n");
    printf("Type 'exit' to quit\n\n");

    while (1) {
        printf("[42]$ ");

        int i = 0;
        char c;
        while (read(0, &c, 1) == 1) {
            if (c == '\n') {
                line[i] = '\0';
                printf("\n");
                break;
            } else if (c == '\b' || c == 127) {
                if (i > 0) {
                    i--;
                    printf("\b \b");
                }
            } else if (c >= 32 && c < 127) {
                if (i < MAX_LINE - 1) {
                    line[i++] = c;
                    write(1, &c, 1);
                }
            }
        }

        if (i == 0)
            continue;

        int argc = parse(line, argv);
        if (argc == 0)
            continue;

        if (strcmp(argv[0], "exit") == 0) {
            printf("Goodbye!\n");
            exit(0);
        }

        if (strcmp(argv[0], "pwd") == 0) {
            builtin_pwd();
            continue;
        }

        if (strcmp(argv[0], "cd") == 0) {
            builtin_cd(argc, argv);
            continue;
        }

        if (strcmp(argv[0], "help") == 0) {
            print_help();
            continue;
        }

        pid_t pid = fork();
        if (pid == 0) {
            int ret = execve(argv[0], argv, 0);

            if (ret == -EINVAL || ret == -ENOENT) {
                printf("Command not found: %s\n", argv[0]);
            } else if (ret == -ENOMEM) {
                printf("Kernel out of memory\n");
            } else {
                printf("Error %d was returned\n", ret);
            }

            exit(1);
        } else if (pid > 0) {
            int status;
            waitpid(&status);
        } else {
            printf("Fork failed!\n");
        }
    }

    return 0;
}
