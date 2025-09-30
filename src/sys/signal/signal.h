#ifndef SIGNAL_H
#define SIGNAL_H

#include "sys/process.h"

#include <stdint.h>

#define _NSIG 32
#define NSIG _NSIG

#define SIGHUP 1
#define SIGINT 2
#define SIGQUIT 3
#define SIGILL 4
#define SIGTRAP 5
#define SIGABRT 6
#define SIGIOT 6
#define SIGUNUSED 7
#define SIGFPE 8
#define SIGKILL 9
#define SIGUSR1 10
#define SIGSEGV 11
#define SIGUSR2 12
#define SIGPIPE 13
#define SIGALRM 14
#define SIGTERM 15
#define SIGSTKFLT 16
#define SIGCHLD 17
#define SIGCONT 18
#define SIGSTOP 19
#define SIGTSTP 20
#define SIGTTIN 21
#define SIGTTOU 22

#define SIGIO 23
#define SIGPOLL SIGIO
#define SIGURG SIGIO
#define SIGXCPU 24
#define SIGXFSZ 25

#define SIGVTALRM 26
#define SIGPROF 27

#define SIGWINCH 28

// #define SIGLOST		29
#define SIGPWR 30

/* Arggh. Bad user source code wants this.. */
#define SIGBUS SIGUNUSED

typedef uint32_t sigset_t;
typedef void (*__sighandler_t)(int32_t);

#define SIG_DFL ((__sighandler_t)0)    /* default signal handling */
#define SIG_IGN ((__sighandler_t)1)    /* ignore signal */
#define SIG_ERR ((__sighandler_t) - 1) /* error return from signal */

typedef struct {
    __sighandler_t sa_handler;
    sigset_t sa_mask;
    int32_t sa_flags;
    void (*sa_restorer)(void);
} sigaction_t;

int32_t do_kill(pid_t pid, int32_t sig);

void handle_signal(void);

#endif /* SIGNAL_H */
