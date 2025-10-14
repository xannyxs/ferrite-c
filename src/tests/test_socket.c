#ifdef __TEST
#    include "drivers/printk.h"
#    include "lib/string.h"
#    include "net/socket.h"
#    include "net/unix.h"
#    include "sys/process/process.h"
#    include "tests/tests.h"
#    include "types.h"
#    include <stdbool.h>

extern u32 tests_passed;
extern u32 tests_failed;

/* Helper to make syscalls from kernel mode */
static inline int k_socket(int family, int type, int protocol)
{
    int ret;
    __asm__ volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(41), "b"(family), "c"(type), "d"(protocol) // SYS_SOCKET = 41
    );
    return ret;
}

static inline int k_bind(int sockfd, void* addr, int addrlen)
{
    int ret;
    __asm__ volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(49), "b"(sockfd), "c"(addr), "d"(addrlen) // SYS_BIND = 49
    );
    return ret;
}

static inline int k_listen(int sockfd, int backlog)
{
    int ret;
    __asm__ volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(50), "b"(sockfd), "c"(backlog) // SYS_LISTEN = 50
    );
    return ret;
}

static inline int k_connect(int sockfd, void* addr, int addrlen)
{
    int ret;
    __asm__ volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(42), "b"(sockfd), "c"(addr), "d"(addrlen) // SYS_CONNECT = 42
    );
    return ret;
}

static inline int k_accept(int sockfd)
{
    int ret;
    __asm__ volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(43), "b"(sockfd), "c"(0), "d"(0) // SYS_ACCEPT = 43
    );
    return ret;
}

static inline int k_read(int fd, void* buf, size_t len)
{
    int ret;
    __asm__ volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(3), "b"(fd), "c"(buf), "d"(len) // SYS_READ = 3
    );
    return ret;
}

static inline int k_write(int fd, void const* buf, size_t len)
{
    int ret;
    __asm__ volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(4), "b"(fd), "c"(buf), "d"(len) // SYS_WRITE = 4
    );
    return ret;
}

static inline void k_exit(int status)
{
    __asm__ volatile(
        "int $0x80"
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
    return true;
}

/* Test 2: Server process - binds and listens */
static void server_process_simple(void)
{
    struct sockaddr_un addr = {
        .sun_family = AF_UNIX,
        .sun_path = "/tmp/test_simple"
    };

    // Create socket
    int sockfd = k_socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printk("  [SERVER] Failed to create socket\n");
        test_result = -1;
        k_exit(1);
    }

    // Bind
    if (k_bind(sockfd, &addr, sizeof(addr)) < 0) {
        printk("  [SERVER] Failed to bind\n");
        test_result = -1;
        k_exit(1);
    }
    printk("  [SERVER] Bound to %s\n", addr.sun_path);

    // Listen
    if (k_listen(sockfd, 5) < 0) {
        printk("  [SERVER] Failed to listen\n");
        test_result = -1;
        k_exit(1);
    }
    printk("  [SERVER] Listening...\n");

    server_ready = 1;

    // Wait a bit for client
    for (int volatile i = 0; i < 5000000; i++)
        ;

    // Accept
    int client_fd = k_accept(sockfd);
    if (client_fd < 0) {
        printk("  [SERVER] Failed to accept\n");
        test_result = -1;
        k_exit(1);
    }
    printk("  [SERVER] Accepted connection on fd=%d\n", client_fd);

    // Receive message
    char buf[100];
    int n = k_read(client_fd, buf, sizeof(buf));
    if (n <= 0) {
        printk("  [SERVER] Failed to read\n");
        test_result = -1;
        k_exit(1);
    }

    buf[n] = '\0';
    printk("  [SERVER] Received: '%s'\n", buf);

    // Send reply
    char const* reply = "ACK";
    k_write(client_fd, reply, strlen(reply));
    printk("  [SERVER] Sent reply\n");

    test_result = 1;
    k_exit(0);
}

static void client_process_simple(void)
{
    while (!server_ready) {
        for (int volatile i = 0; i < 100000; i++)
            ;
    }

    struct sockaddr_un addr = {
        .sun_family = AF_UNIX,
        .sun_path = "/tmp/test_simple"
    };

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

    char const* msg = "Hello from client";
    int n = k_write(sockfd, msg, strlen(msg));
    if (n != (int)strlen(msg)) {
        printk("  [CLIENT] Failed to write\n");
        test_result = -1;
        k_exit(1);
    }
    printk("  [CLIENT] Sent: '%s'\n", msg);

    // Receive reply
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

/* Test 2: Basic client-server communication */
TEST(socket_client_server)
{
    printk("  Testing client-server communication...\n");

    server_ready = 0;
    client_done = 0;
    test_result = 0;

    // Start server
    pid_t server_pid = do_exec("socket_server", server_process_simple);
    ASSERT(server_pid > 0, "Should create server process");

    // Start client
    pid_t client_pid = do_exec("socket_client", client_process_simple);
    ASSERT(client_pid > 0, "Should create client process");

    // Wait for both to complete
    for (int i = 0; i < 100 && !client_done; i++) {
        for (int volatile j = 0; j < 1000000; j++)
            ;
    }

    ASSERT(test_result == 1, "Communication should succeed");

    return true;
}

/* Test 3: Multiple messages */
static void server_process_multi(void)
{
    struct sockaddr_un addr = {
        .sun_family = AF_UNIX,
        .sun_path = "/tmp/test_multi"
    };

    int sockfd = k_socket(AF_UNIX, SOCK_STREAM, 0);
    k_bind(sockfd, &addr, sizeof(addr));
    k_listen(sockfd, 5);

    printk("  [SERVER-MULTI] Listening...\n");
    server_ready = 1;

    for (int volatile i = 0; i < 5000000; i++)
        ;

    int client_fd = k_accept(sockfd);
    printk("  [SERVER-MULTI] Accepted connection\n");

    // Receive multiple messages
    char buf[50];
    for (int i = 0; i < 3; i++) {
        int n = k_read(client_fd, buf, sizeof(buf));
        if (n > 0) {
            buf[n] = '\0';
            printk("  [SERVER-MULTI] Message %d: '%s'\n", i + 1, buf);
        }
    }

    test_result = 1;
    k_exit(0);
}

static void client_process_multi(void)
{
    while (!server_ready) {
        for (int volatile i = 0; i < 100000; i++)
            ;
    }

    struct sockaddr_un addr = {
        .sun_family = AF_UNIX,
        .sun_path = "/tmp/test_multi"
    };

    int sockfd = k_socket(AF_UNIX, SOCK_STREAM, 0);
    k_connect(sockfd, &addr, sizeof(addr));

    printk("  [CLIENT-MULTI] Sending messages...\n");

    // Send multiple messages
    k_write(sockfd, "Message 1", 9);
    k_write(sockfd, "Message 2", 9);
    k_write(sockfd, "Message 3", 9);

    client_done = 1;
    k_exit(0);
}

TEST(socket_multiple_messages)
{
    printk("  Testing multiple messages...\n");

    server_ready = 0;
    client_done = 0;
    test_result = 0;

    pid_t server_pid = do_exec("socket_server_multi", server_process_multi);
    ASSERT(server_pid > 0, "Should create server");

    pid_t client_pid = do_exec("socket_client_multi", client_process_multi);
    ASSERT(client_pid > 0, "Should create client");

    for (int i = 0; i < 100 && !client_done; i++) {
        for (int volatile j = 0; j < 1000000; j++)
            ;
    }

    ASSERT(test_result == 1, "Multiple messages should work");

    return true;
}

TEST(socket_connection_refused)
{
    printk("  Testing connection refused...\n");

    struct sockaddr_un addr = {
        .sun_family = AF_UNIX,
        .sun_path = "/tmp/nonexistent"
    };

    int sockfd = k_socket(AF_UNIX, SOCK_STREAM, 0);
    ASSERT(sockfd >= 0, "socket() should succeed");

    int result = k_connect(sockfd, &addr, sizeof(addr));
    ASSERT(result < 0, "connect() should fail to nonexistent socket");

    printk("  Connection correctly refused\n");
    return true;
}

void socket_tests(void)
{
    printk("\n========== UNIX SOCKET TEST SUITE ==========\n\n");

    RUN_TEST(socket_create);
    RUN_TEST(socket_connection_refused);
    RUN_TEST(socket_client_server);
    // RUN_TEST(socket_multiple_messages);

    printk("\n============================================\n");
}

#endif
