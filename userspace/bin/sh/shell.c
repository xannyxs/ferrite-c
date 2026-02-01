#include "uapi/fcntl.h"
#include "uapi/reboot.h"
#include <libc/stdio.h>
#include <libc/string.h>
#include <libc/syscalls.h>
#include <uapi/errno.h>

#define MAX_LINE 256
#define MAX_ARGS 32

#define MAX_ENV 32
#define MAX_ENV_LEN 256

static char env_vars[MAX_ENV][MAX_ENV_LEN];
static int env_count = 0;

/* Private */

static void init_env(void)
{
    strlcpy(env_vars[env_count++], "PATH=/bin:/sbin", MAX_ENV_LEN);
    strlcpy(env_vars[env_count++], "HOME=/", MAX_ENV_LEN);
    strlcpy(env_vars[env_count++], "USER=root", MAX_ENV_LEN);
}

static char* getenv(char const* name)
{
    size_t len = strlen(name);

    for (int i = 0; i < env_count; i++) {
        if (strncmp(env_vars[i], name, len) == 0 && env_vars[i][len] == '=') {
            return &env_vars[i][len + 1];
        }
    }

    return NULL;
}

static int setenv(char const* name, char const* value)
{
    size_t len = strlen(name);

    for (int i = 0; i < env_count; i++) {
        if (strncmp(env_vars[i], name, len) == 0 && env_vars[i][len] == '=') {
            (void)snprintf(env_vars[i], MAX_ENV_LEN, "%s=%s", name, value);
            return 0;
        }
    }

    if (env_count >= MAX_ENV) {
        return -1;
    }

    (void)snprintf(env_vars[env_count++], MAX_ENV_LEN, "%s=%s", name, value);
    return 0;
}

static int find_in_path(char const* cmd, char* fullpath, int maxlen)
{
    if (strchr(cmd, '/')) {
        strlcpy(fullpath, cmd, maxlen);
        return 0;
    }

    char const* path = getenv("PATH");
    if (!path) {
        return -1;
    }

    char path_copy[256];
    strlcpy(path_copy, path, sizeof(path_copy));
    path_copy[sizeof(path_copy) - 1] = '\0';

    char* token = path_copy;
    char* next;

    while (token) {
        next = strchr(token, ':');
        if (next) {
            *next = '\0';
            next++;
        }

        (void)snprintf(fullpath, maxlen, "%s/%s", token, cmd);

        struct stat st;
        if (stat(fullpath, &st) == 0) {
            return 0;
        }

        token = next;
    }

    return -1;
}

static void print_help(void)
{
    printf("\n");
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║            Ferrite Kernel Shell - Help                ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n\n");

    printf("BUILTINS\n");
    printf("  cd <directory>              Change working directory\n");
    printf("  pwd                         Print working directory\n");
    printf("  export VAR=value            Set environment variable\n");
    printf("  env                         Print all environment variables\n");
    printf("  exit                        Exit the shell\n");
    printf("  help                        Show this help\n");
    printf("\n");

    printf("FILE OPERATIONS\n");
    printf("  ls [path]                   List directory\n");
    printf("  cat <file>                  Display file contents\n");
    printf("  touch <file>                Create empty file\n");
    printf("  rm <file>                   Delete file\n");
    printf("  mkdir <directory>           Create directory\n");
    printf("  rmdir <directory>           Remove directory\n");
    printf("\n");
}

static int builtin_echo(int argc, char** argv)
{
    int redirect_type = 0;
    int redirect_idx = -1;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], ">") == 0) {
            redirect_type = 1;
            redirect_idx = i;
            break;
        } else if (strcmp(argv[i], ">>") == 0) {
            redirect_type = 2;
            redirect_idx = i;
            break;
        }
    }

    if (redirect_type == 0) {
        for (int i = 1; i < argc; i++) {
            printf("%s", argv[i]);
            if (i < argc - 1)
                printf(" ");
        }
        printf("\n");
        return 0;
    }

    if (redirect_idx + 1 >= argc) {
        printf("echo: missing filename\n");
        return 1;
    }

    char const* filename = argv[redirect_idx + 1];

    char text[512];
    int pos = 0;
    for (int i = 1; i < redirect_idx; i++) {
        size_t len = strlen(argv[i]);
        if (pos + len + 1 >= sizeof(text)) {
            break;
        }

        if (i > 1)
            text[pos++] = ' ';
        memcpy(text + pos, argv[i], len);
        pos += len;
    }
    text[pos++] = '\n';

    int flags = O_CREAT | O_WRONLY;
    int fd = open(filename, flags, 0644);
    if (fd < 0) {
        printf("echo: cannot write to '%s'\n", filename);
        return 1;
    }

    if (redirect_type == 2) {
        lseek(fd, 0, SEEK_END);
    }

    write(fd, text, pos);
    close(fd);

    return 0;
}

static int builtin_cd(int argc, char** argv)
{
    if (argc < 2) {
        char const* home = getenv("HOME");
        if (home && chdir(home) == 0) {
            return 0;
        }
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
    return 1;
}

static int builtin_export(int argc, char** argv)
{
    if (argc < 2) {
        printf("Usage: export VAR=value\n");
        return 1;
    }

    char* equals = strchr(argv[1], '=');
    if (!equals) {
        printf("export: invalid format, use VAR=value\n");
        return 1;
    }

    *equals = '\0';
    char const* name = argv[1];
    char const* value = equals + 1;

    if (setenv(name, value) < 0) {
        printf("export: too many environment variables\n");
        return 1;
    }

    return 0;
}

static int builtin_env(void)
{
    for (int i = 0; i < env_count; i++) {
        printf("%s\n", env_vars[i]);
    }
    return 0;
}

static int builtin_reboot(void)
{
    reboot(
        FERRITE_REBOOT_MAGIC1, FERRITE_REBOOT_MAGIC2,
        FERRITE_REBOOT_CMD_RESTART, NULL
    );
    return 0;
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

    init_env();

    printf("Ferrite Shell v0.1\n");
    printf("Type 'help' for commands, 'exit' to quit\n\n");

    while (1) {
        printf("[42]$ ");

        int i = 0;
        char c;
        while (read(0, &c, 1) == 1) {
            if (c == '\n') {
                line[i] = '\0';
                printf("\n");
                break;
            }

            if (c == '\b' || c == 127) {
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

        // Builtins
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

        if (strcmp(argv[0], "export") == 0) {
            builtin_export(argc, argv);
            continue;
        }

        if (strcmp(argv[0], "env") == 0) {
            builtin_env();
            continue;
        }

        if (strcmp(argv[0], "echo") == 0) {
            builtin_echo(argc, argv);
            continue;
        }

        if (strcmp(argv[0], "reboot") == 0) {
            builtin_reboot();
            continue;
        }

        if (strcmp(argv[0], "clear") == 0) {
            // printf("\033[2J\033[H");
            continue;
        }

        if (strcmp(argv[0], "help") == 0) {
            print_help();
            continue;
        }

        char fullpath[256];
        if (find_in_path(argv[0], fullpath, sizeof(fullpath)) < 0) {
            printf("Command not found: %s\n", argv[0]);
            continue;
        }

        pid_t pid = fork();
        if (pid == 0) {
            int ret = execve(fullpath, argv, 0);

            if (ret == -ENOENT) {
                printf("%s: command not found\n", argv[0]);
            } else if (ret == -ENOMEM) {
                printf("Kernel out of memory\n");
            } else {
                printf("Error executing %s: %d\n", argv[0], ret);
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
