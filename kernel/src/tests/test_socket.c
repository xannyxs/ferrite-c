#ifdef __TEST
#    include "arch/x86/idt/syscalls.h"
#    include "drivers/printk.h"
#    include "ferrite/string.h"
#    include "net/socket.h"
#    include "net/unix.h"
#    include "sys/process/process.h"
#    include "sys/timer/timer.h"
#    include "tests/tests.h"

#    include <ferrite/types.h>
#    include <stdbool.h>

extern u32 tests_passed;
extern u32 tests_failed;

static inline int k_socket(int family, int type, int protocol)
{
    int ret;
    unsigned long args[3];
    args[0] = family;
    args[1] = type;
    args[2] = protocol;

    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(SYS_SOCKETCALL), "b"(SYS_SOCKET), "c"(args)
                     : "memory");
    return ret;
}

static inline int k_bind(int sockfd, void* addr, int addrlen)
{
    int ret;
    unsigned long args[3] = { sockfd, (unsigned long)addr, addrlen };
    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(SYS_SOCKETCALL), "b"(SYS_BIND), "c"(args)
                     : "memory");
    return ret;
}

static inline int k_listen(int sockfd, int backlog)
{
    int ret;
    unsigned long args[2] = { sockfd, backlog };
    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(SYS_SOCKETCALL), "b"(SYS_LISTEN), "c"(args)
                     : "memory");
    return ret;
}

static inline int k_connect(int sockfd, void* addr, int addrlen)
{
    int ret;
    unsigned long args[3] = { sockfd, (unsigned long)addr, addrlen };
    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(SYS_SOCKETCALL), "b"(SYS_CONNECT), "c"(args)
                     : "memory");
    return ret;
}

static inline int k_accept(int sockfd)
{
    int ret;
    unsigned long args[3] = { sockfd, 0, 0 };
    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(SYS_SOCKETCALL), "b"(SYS_ACCEPT), "c"(args)
                     : "memory");
    return ret;
}

static inline int k_read(int fd, void* buf, size_t len)
{
    int ret;
    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(3), "b"(fd), "c"(buf), "d"(len) // SYS_READ = 3
    );
    return ret;
}

static inline int k_write(int fd, void const* buf, size_t len)
{
    int ret;
    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(4), "b"(fd), "c"(buf), "d"(len) // SYS_WRITE = 4
    );
    return ret;
}

static inline void k_exit(int status)
{
    __asm__ volatile("int $0x80"
                     :
                     : "a"(1), "b"(status) // SYS_EXIT = 1
    );
    __builtin_unreachable();
}

static int volatile server_ready = 0;
static int volatile client_done = 0;
static int volatile test_result = 0;

TEST(socket_create)
{
    printk("  Creating Unix stream socket...\n");

    int sockfd = k_socket(AF_UNIX, SOCK_STREAM, 0);
    ASSERT(sockfd >= 0, "socket() should succeed");

    printk("  Created socket with fd=%d\n", sockfd);
    do_exit(0);
}

static void server_process_simple(void)
{
    struct sockaddr_un addr
        = { .sun_family = AF_UNIX, .sun_path = "/tmp/test_simple" };

    int sockfd = k_socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printk("  [SERVER] Failed to create socket\n");
        test_result = -1;
        k_exit(1);
    }
    int bind_result = k_bind(sockfd, &addr, sizeof(addr));
    if (bind_result < 0) {
        printk("  [SERVER] Failed to bind\n");
        test_result = -1;
        k_exit(1);
    }

    if (k_listen(sockfd, 5) < 0) {
        printk("  [SERVER] Failed to listen\n");
        test_result = -1;
        k_exit(1);
    }
    printk("  [SERVER] Listening...\n");

    server_ready = 1;

    int client_fd = k_accept(sockfd);
    if (client_fd < 0) {
        printk("  [SERVER] Failed to accept\n");
        test_result = -1;
        k_exit(1);
    }
    printk("  [SERVER] Accepted connection on fd=%d\n", client_fd);

    yield();

    char buf[100];
    int n = k_read(client_fd, buf, sizeof(buf));
    if (n <= 0) {
        printk("  [SERVER] Failed to read\n");
        test_result = -1;
        k_exit(1);
    }

    buf[n] = '\0';
    printk("  [SERVER] Received: '%s'\n", buf);

    char const* reply = "ACK";
    k_write(client_fd, reply, strlen(reply));
    printk("  [SERVER] Sent reply\n");

    yield();
    test_result = 1;
    k_exit(0);
}

static void client_process_simple(void)
{
    struct sockaddr_un addr
        = { .sun_family = AF_UNIX, .sun_path = "/tmp/test_simple" };

    printk("  [CLIENT] Server is ready, connecting...\n");

    int sockfd = k_socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printk("  [CLIENT] Failed to create socket\n");
        test_result = -1;
        k_exit(1);
    }

    if (k_connect(sockfd, &addr, sizeof(addr)) < 0) {
        printk("  [CLIENT] Failed to connect\n");
        test_result = -1;
        k_exit(1);
    }
    printk("  [CLIENT] Connected to %s\n", addr.sun_path);
    yield();

    char const* msg = "Hello from client";
    int n = k_write(sockfd, msg, strlen(msg));
    if (n != (int)strlen(msg)) {
        printk("  [CLIENT] Failed to write\n");
        test_result = -1;
        k_exit(1);
    }
    printk("  [CLIENT] Sent: '%s'\n", msg);

    yield();
    char buf[100];
    n = k_read(sockfd, buf, sizeof(buf));
    if (n <= 0) {
        printk("  [CLIENT] Failed to read reply\n");
        test_result = -1;
        k_exit(1);
    }

    buf[n] = '\0';
    printk("  [CLIENT] Received reply: '%s'\n", buf);

    client_done = 1;
    k_exit(0);
}

TEST(socket_client_server)
{
    printk("  Testing client-server communication...\n");

    server_ready = 0;
    client_done = 0;
    test_result = 0;

    pid_t server_pid = do_exec("socket_server", server_process_simple);
    ASSERT(server_pid > 0, "Should create server process");
    yield();

    pid_t client_pid = do_exec("socket_client", client_process_simple);
    ASSERT(client_pid > 0, "Should create client process");
    do_wait(0);
    do_wait(0);

    ASSERT(test_result == 1, "Communication should succeed");

    do_exit(0);
}

static void server_process_multi(void)
{
    struct sockaddr_un addr
        = { .sun_family = AF_UNIX, .sun_path = "/tmp/test_multi" };

    int sockfd = k_socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printk("  [SERVER] Failed to create socket\n");
        test_result = -1;
        k_exit(1);
    }
    int bind_result = k_bind(sockfd, &addr, sizeof(addr));
    if (bind_result < 0) {
        printk("  [SERVER] Failed to bind\n");
        test_result = -1;
        k_exit(1);
    }

    if (k_listen(sockfd, 5) < 0) {
        printk("  [SERVER] Failed to listen\n");
        test_result = -1;
        k_exit(1);
    }
    printk("  [SERVER] Listening...\n");

    server_ready = 1;

    yield();

    int client_fd = k_accept(sockfd);
    if (client_fd < 0) {
        printk("  [SERVER] Failed to accept\n");
        test_result = -1;
        k_exit(1);
    }
    printk("  [SERVER] Accepted connection on fd=%d\n", client_fd);

    yield();
    char buf[100];
    int n = k_read(client_fd, buf, sizeof(buf));
    if (n <= 0) {
        printk("  [SERVER] Failed to read\n");
        test_result = -1;
        k_exit(1);
    }

    buf[n] = '\0';
    printk("  [SERVER] Received: '%s'\n", buf);

    char const* reply = "ACK";
    k_write(client_fd, reply, strlen(reply));
    printk("  [SERVER] Sent reply\n");

    yield();
    test_result = 1;
    k_exit(0);
}

static void client_process_multi(void)
{
    struct sockaddr_un addr
        = { .sun_family = AF_UNIX, .sun_path = "/tmp/test_multi" };

    int sockfd = k_socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printk("  [CLIENT] Failed to create socket\n");
        test_result = -1;
        k_exit(1);
    }

    if (k_connect(sockfd, &addr, sizeof(addr)) < 0) {
        printk("  [CLIENT] Failed to connect\n");
        test_result = -1;
        k_exit(1);
    }
    printk("  [CLIENT] Connected to %s\n", addr.sun_path);
    yield();

    char const* msg = "Hello from client";
    int n = k_write(sockfd, msg, strlen(msg));
    if (n != (int)strlen(msg)) {
        printk("  [CLIENT] Failed to write\n");
        test_result = -1;
        k_exit(1);
    }
    printk("  [CLIENT] Sent: '%s'\n", msg);

    yield();
    char buf[100];
    n = k_read(sockfd, buf, sizeof(buf));
    if (n <= 0) {
        printk("  [CLIENT] Failed to read reply\n");
        test_result = -1;
        k_exit(1);
    }

    buf[n] = '\0';
    printk("  [CLIENT] Received reply: '%s'\n", buf);

    client_done = 1;
    k_exit(0);
}

TEST(socket_multiple_messages)
{
    printk("  Testing multiple messages...\n");
    printk(
        "  Global state: server_ready=%d, client_done=%d, test_result=%d\n",
        server_ready, client_done, test_result
    );

    server_ready = 0;
    client_done = 0;
    test_result = 0;

    pid_t server_pid = do_exec("socket_server_multi", server_process_multi);
    ASSERT(server_pid > 0, "Should create server");
    yield();

    pid_t client_pid = do_exec("socket_client_multi", client_process_multi);
    ASSERT(client_pid > 0, "Should create client");
    do_wait(0);
    do_wait(0);

    process_list();
    ASSERT(test_result == 1, "Multiple messages should work");

    do_exit(0);
}

TEST(socket_connection_refused)
{
    printk("  Testing connection refused...\n");

    struct sockaddr_un addr
        = { .sun_family = AF_UNIX, .sun_path = "/tmp/nonexistent" };

    int sockfd = k_socket(AF_UNIX, SOCK_STREAM, 0);
    ASSERT(sockfd >= 0, "socket() should succeed");

    int result = k_connect(sockfd, &addr, sizeof(addr));
    ASSERT(result < 0, "connect() should fail to nonexistent socket");

    printk("  Connection correctly refused\n");
    do_exit(0);
}

void socket_tests(void)
{
    printk("\n========== UNIX SOCKET TEST SUITE ==========\n\n");

    RUN_TEST(socket_create);
    RUN_TEST(socket_connection_refused);
    RUN_TEST(socket_client_server);
    RUN_TEST(socket_multiple_messages);

    printk("\n============================================\n");
}

#endif

typedef int _test_translation_unit_not_empty;
