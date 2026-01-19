#include "../../lib/libc/stdio/stdio.h"
#include "../../lib/libc/syscalls.h"

#define MAX_LINE 256
#define MAX_ARGS 32

int parse(char* line, char** argv)
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

        if (strcmp(argv[0], "help") == 0) {
            printf("Available commands:\n");
            printf("  help  - Show this message\n");
            printf("  exit  - Exit shell\n");
            printf("  hello - Test program\n");
            continue;
        }

        pid_t pid = fork();
        if (pid == 0) {
            execve(argv[0], argv, 0);

            printf("Command not found: ");
            printf(argv[0]);
            printf("\n");
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
