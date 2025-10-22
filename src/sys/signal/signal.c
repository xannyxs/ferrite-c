#include "sys/signal/signal.h"
#include "drivers/printk.h"
#include "sys/process/process.h"
#include "types.h"

extern proc_t ptables[NUM_PROC];
extern proc_t* current_proc;

sigaction_t sigaction[NSIG] = {
    [SIGHUP] = { .sa_handler = SIG_DFL, .sa_flags = 0 },
    [SIGINT] = { .sa_handler = SIG_DFL, .sa_flags = 0 },
    [SIGQUIT] = { .sa_handler = SIG_DFL, .sa_flags = 0 },
    [SIGILL] = { .sa_handler = SIG_DFL, .sa_flags = 0 },
    [SIGTRAP] = { .sa_handler = SIG_DFL, .sa_flags = 0 },
    [SIGABRT] = { .sa_handler = SIG_DFL, .sa_flags = 0 },
    [SIGUNUSED] = { .sa_handler = SIG_IGN, .sa_flags = 0 },
    [SIGFPE] = { .sa_handler = SIG_DFL, .sa_flags = 0 },

    [SIGKILL] = { .sa_handler = SIG_DFL, .sa_flags = 0 },
    [SIGUSR1] = { .sa_handler = SIG_IGN, .sa_flags = 0 },
    [SIGSEGV] = { .sa_handler = SIG_IGN, .sa_flags = 0 },
    [SIGUSR2] = { .sa_handler = SIG_IGN, .sa_flags = 0 },
    [SIGPIPE] = { .sa_handler = SIG_IGN, .sa_flags = 0 },
    [SIGALRM] = { .sa_handler = SIG_IGN, .sa_flags = 0 },
    [SIGTERM] = { .sa_handler = SIG_DFL, .sa_flags = 0 },
    [SIGSTKFLT] = { .sa_handler = SIG_DFL, .sa_flags = 0 },

    [SIGCHLD] = { .sa_handler = SIG_IGN, .sa_flags = 0 },
    [SIGCONT] = { .sa_handler = SIG_DFL, .sa_flags = 0 },
    [SIGSTOP] = { .sa_handler = SIG_DFL, .sa_flags = 0 },
    [SIGTSTP] = { .sa_handler = SIG_DFL, .sa_flags = 0 },
    [SIGTTIN] = { .sa_handler = SIG_DFL, .sa_flags = 0 },
    [SIGTTOU] = { .sa_handler = SIG_DFL, .sa_flags = 0 },
    [SIGIO] = { .sa_handler = SIG_IGN, .sa_flags = 0 },

    [SIGXCPU] = { .sa_handler = SIG_DFL, .sa_flags = 0 },
    [SIGXFSZ] = { .sa_handler = SIG_DFL, .sa_flags = 0 },
    [SIGVTALRM] = { .sa_handler = SIG_DFL, .sa_flags = 0 },
    [SIGPROF] = { .sa_handler = SIG_DFL, .sa_flags = 0 },
    [SIGWINCH] = { .sa_handler = SIG_IGN, .sa_flags = 0 },
    [SIGPWR] = { .sa_handler = SIG_DFL, .sa_flags = 0 },
};

/* Private */

static inline void __default_sigterm_handler(s32 sig)
{
    printk("Process %d terminated by signal %d\n", current_proc->pid, sig);
    current_proc->state = ZOMBIE;
    yield();
}

static inline void __default_sigkill_handler(s32 sig)
{
    printk("Process %d killed by signal %d\n", current_proc->pid, sig);
    current_proc->state = ZOMBIE;
    yield();
}

static inline void __default_sigcore_handler(s32 sig)
{
    printk("Process %d core dumped by signal %d\n", current_proc->pid, sig);
    // TODO: Dump core
    current_proc->state = ZOMBIE;
    yield();
}

static inline void __default_sigstop_handler(s32 sig)
{
    printk("Process %d stopped by signal %d\n", current_proc->pid, sig);
    current_proc->state = SLEEPING;
    yield();
}

/* Public */

s32 do_kill(pid_t pid, s32 sig)
{
    if (sig < 1 || sig > NSIG) {
        return -1;
    }

    proc_t* p = find_process(pid);
    if (!p) {
        return -1;
    }

    p->pending_signals |= (1 << sig);

    if (p->state == SLEEPING) {
        p->state = READY;
    }

    printk("Process %d sent signal %d to process %d\n", current_proc->pid, sig,
        pid);

    return 0;
}

void handle_signal(void)
{
    if (!current_proc->pending_signals) {
        return;
    }

    for (s32 sig = 1; sig < NSIG; sig += 1) {
        if (!(current_proc->pending_signals & (1 << sig))) {
            continue;
        }

        current_proc->pending_signals &= ~(1 << sig);
        __sighandler_t handler = sigaction[sig].sa_handler;

        // TODO: Handle SIG_ERR instead of continuing
        if (handler == SIG_IGN || handler == SIG_ERR) {
            continue;
        }

        if (!handler && handler == SIG_DFL) {
            switch (sig) {
            case SIGSTOP:
            case SIGTSTP:
            case SIGTTIN:
            case SIGTTOU:
                __default_sigstop_handler(sig);
                break;

            case SIGKILL:
            case SIGTERM:
            case SIGINT:
                __default_sigkill_handler(sig);
                break;

            case SIGCONT:
                if (current_proc->state == SLEEPING) {
                    current_proc->state = READY;
                }
                break;

            case SIGILL:
            case SIGTRAP:
            case SIGABRT:
            case SIGFPE:
            case SIGSEGV:
            case SIGBUS:
                __default_sigcore_handler(sig);
                break;

            case SIGPIPE:
            case SIGALRM:
            case SIGUSR2:
            case SIGHUP:
            case SIGQUIT:
                __default_sigterm_handler(sig);
                break;

            default:
                // Unknown signal
                printk("Process %d: unknown signal %d, ignoring\n",
                    current_proc->pid, sig);
                break;
            }
        }

        printk("Process %d handling signal %d with custom handler\n",
            current_proc->pid, sig);
        handler(sig);
    }
}
