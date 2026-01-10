// userspace/bin/hello/hello.c
#include "../../lib/libc/syscalls.h"

void print(char const* s) { write(1, s, strlen(s)); }

int main(int argc, char** argv)
{
    print("Hello from userspace!\n");
    print("PID: ");

    // Print PID (simple integer to string)
    int pid = getpid();
    char buf[16];
    int i = 0;

    if (pid == 0) {
        buf[i++] = '0';
    } else {
        int tmp = pid;
        int digits = 0;
        while (tmp > 0) {
            digits++;
            tmp /= 10;
        }
        for (int j = digits - 1; j >= 0; j--) {
            buf[j] = '0' + (pid % 10);
            pid /= 10;
        }
        i = digits;
    }
    buf[i] = '\0';

    print(buf);
    print("\n");

    if (argc > 1) {
        print("Arguments:\n");
        for (int j = 1; j < argc; j++) {
            print("  ");
            print(argv[j]);
            print("\n");
        }
    }

    return 42;
}
