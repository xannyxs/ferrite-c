#include "../../lib/libc/syscalls.h"

#define MAX_LINE 256
#define MAX_ARGS 32

void print(char const* s) { write(1, s, strlen(s)); }

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

    print("Ferrite Shell v0.1\n");
    print("Type 'exit' to quit\n\n");

    while (1) {
        print("[42]$ ");

        int i = 0;
        char c;
        while (read(0, &c, 1) == 1) {
            if (c == '\n') {
                line[i] = '\0';
                print("\n");
                break;
            } else if (c == '\b' || c == 127) {
                if (i > 0) {
                    i--;
                    print("\b \b");
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
            print("Goodbye!\n");
            exit(0);
        }

        if (strcmp(argv[0], "help") == 0) {
            print("Available commands:\n");
            print("  help  - Show this message\n");
            print("  exit  - Exit shell\n");
            print("  hello - Test program\n");
            continue;
        }

        pid_t pid = fork();
        if (pid == 0) {
            print("Child\n");
            execve(argv[0], argv, 0);

            print("Command not found: ");
            print(argv[0]);
            print("\n");
            exit(1);
        } else if (pid > 0) {
            print("Parent\n");

            int status;
            waitpid(&status);
        } else {
            print("Fork failed!\n");
        }
    }

    return 0;
}
