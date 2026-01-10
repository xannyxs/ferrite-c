#include "../../lib/libc/syscalls.h"

#define MAX_LINE 256
#define MAX_ARGS 32

void print(char const* s) { write(1, s, strlen(s)); }

// Parse command line into argv
int parse(char* line, char** argv)
{
    int argc = 0;

    while (*line) {
        // Skip whitespace
        while (*line == ' ')
            line++;
        if (!*line)
            break;

        argv[argc++] = line;

        // Find end of word
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
        // Print prompt
        print("[42]$ ");

        // Read line
        int i = 0;
        char c;
        while (read(0, &c, 1) == 1) {
            if (c == '\n') {
                line[i] = '\0';
                print("\n");
                break;
            } else if (c == '\b' || c == 127) { // Backspace
                if (i > 0) {
                    i--;
                    print("\b \b");
                }
            } else if (c >= 32 && c < 127) { // Printable
                if (i < MAX_LINE - 1) {
                    line[i++] = c;
                    write(1, &c, 1);
                }
            }
        }

        if (i == 0)
            continue;

        // Parse command
        int argc = parse(line, argv);
        if (argc == 0)
            continue;

        // Built-in: exit
        if (strcmp(argv[0], "exit") == 0) {
            print("Goodbye!\n");
            exit(0);
        }

        // Built-in: help
        if (strcmp(argv[0], "help") == 0) {
            print("Available commands:\n");
            print("  help  - Show this message\n");
            print("  exit  - Exit shell\n");
            print("  hello - Test program\n");
            continue;
        }

        // Try to execute as external command
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            execve(argv[0], argv, 0);

            // If we get here, exec failed
            print("Command not found: ");
            print(argv[0]);
            print("\n");
            exit(1);
        } else if (pid > 0) {
            int status;
            waitpid(&status);
        } else {
            print("Fork failed!\n");
        }
    }

    return 0;
}
