#include <libc/stdio.h>
#include <libc/string.h>
#include <libc/syscalls.h>
#include <uapi/dirent.h>
#include <uapi/fcntl.h>
#include <uapi/socket.h>
#include <uapi/types.h>

#include <stdbool.h>

#define ASSERT(cond, msg)                                      \
    if (!(cond)) {                                             \
        printf("[FAIL] %s:%d: %s\n", __FILE__, __LINE__, msg); \
        exit(1);                                               \
    }

#define ASSERT_EQ(a, b, message) ASSERT((a) == (b), message)

#define TEST(name) static void __attribute__((unused)) test_##name(void)

#define RUN_TEST(name)                  \
    do {                                \
        printf("[TEST] " #name "... "); \
                                        \
        test_##name();                  \
        printf("PASS\n");               \
        tests_passed++;                 \
    } while (0)

static int tests_passed = 0;
static int tests_failed = 0;

static int volatile server_ready = 0;
static int volatile client_done = 0;
static int volatile test_result = 0;

TEST(socket_create)
{
    printf("  Creating Unix stream socket...\n");

    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    ASSERT(sockfd >= 0, "socket() should succeed");

    printf("  Created socket with fd=%d\n", sockfd);
}

TEST(socket_connection_refused)
{
    printf("  Testing connection refused...\n");

    struct sockaddr_un addr
        = { .sun_family = AF_UNIX, .sun_path = "/tmp/nonexistent" };

    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    ASSERT(sockfd >= 0, "socket() should succeed");

    int result = connect(sockfd, &addr, sizeof(addr));
    ASSERT(result < 0, "connect() should fail to nonexistent socket");

    printf("  Connection correctly refused\n");
}

static void server_process_simple(void)
{
    struct sockaddr_un addr
        = { .sun_family = AF_UNIX, .sun_path = "/tmp/test_simple" };

    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("  [SERVER] Failed to create socket\n");
        test_result = -1;
        exit(1);
    }

    int bind_result = bind(sockfd, &addr, sizeof(addr));
    if (bind_result < 0) {
        printf("  [SERVER] Failed to bind\n");
        test_result = -1;
        exit(1);
    }

    if (listen(sockfd, 5) < 0) {
        printf("  [SERVER] Failed to listen\n");
        test_result = -1;
        exit(1);
    }
    printf("  [SERVER] Listening...\n");

    server_ready = 1;

    int client_fd = accept(sockfd, NULL, 0);
    if (client_fd < 0) {
        printf("  [SERVER] Failed to accept\n");
        test_result = -1;
        exit(1);
    }
    printf("  [SERVER] Accepted connection on fd=%d\n", client_fd);

    char buf[100];
    int n = read(client_fd, buf, sizeof(buf));
    if (n <= 0) {
        printf("  [SERVER] Failed to read\n");
        test_result = -1;
        exit(1);
    }

    buf[n] = '\0';
    printf("  [SERVER] Received: '%s'\n", buf);

    char const* reply = "ACK";
    write(client_fd, reply, strlen(reply));
    printf("  [SERVER] Sent reply\n");

    test_result = 1;
}

static void client_process_simple(void)
{
    struct sockaddr_un addr
        = { .sun_family = AF_UNIX, .sun_path = "/tmp/test_simple" };

    printf("  [CLIENT] Server is ready, connecting...\n");

    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("  [CLIENT] Failed to create socket\n");
        test_result = -1;
        exit(1);
    }

    if (connect(sockfd, &addr, sizeof(addr)) < 0) {
        printf("  [CLIENT] Failed to connect\n");
        test_result = -1;
        exit(1);
    }
    printf("  [CLIENT] Connected to %s\n", addr.sun_path);

    char const* msg = "Hello from client";
    int n = write(sockfd, msg, strlen(msg));
    if (n != (int)strlen(msg)) {
        printf("  [CLIENT] Failed to write\n");
        test_result = -1;
        exit(1);
    }
    printf("  [CLIENT] Sent: '%s'\n", msg);

    char buf[100];
    n = read(sockfd, buf, sizeof(buf));
    if (n <= 0) {
        printf("  [CLIENT] Failed to read reply\n");
        test_result = -1;
        exit(1);
    }

    buf[n] = '\0';
    printf("  [CLIENT] Received reply: '%s'\n", buf);

    client_done = 1;
}

TEST(socket_client_server)
{
    printf("  Testing client-server communication...\n");

    server_ready = 0;
    client_done = 0;
    test_result = 0;

    pid_t server_pid = fork();
    if (server_pid == 0) {
        server_process_simple();
        exit(0);
    }

    // while (!server_ready) {
    //     // usleep(10000);
    // }

    pid_t client_pid = fork();
    if (client_pid == 0) {
        client_process_simple();
        exit(0);
    }

    int status1;
    int status2;

    waitpid(&status1);
    waitpid(&status2);

    ASSERT(test_result == 1, "Communication should succeed");
}

void filesystem_tests(void)
{
    printf("\n========== FILESYSTEM TEST SUITE ==========\n\n");

    RUN_TEST(socket_create);
    RUN_TEST(socket_connection_refused);
    RUN_TEST(socket_client_server);

    printf("\n============================================\n");
}

int main(void)
{
    filesystem_tests();

    printf("\n========== TEST RESULTS ==========\n");
    printf("Passed: %u\n", tests_passed);
    printf("Failed: %u\n", tests_failed);

    if (tests_failed > 0) {
        printf("STATUS: FAILED ❌\n");
    } else {
        printf("STATUS: ALL TESTS PASSED ✅\n");
    }
    printf("====================================\n\n");

    return 0;
}
