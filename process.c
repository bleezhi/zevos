/* ZevOS process/scheduler foundation.
 * The current kernel still runs as one kernel process; these structures
 * define the interface that user-mode processes will use later.
 */

#include <stdint.h>

#define PROCESS_MAX 64
#define PROCESS_UNUSED 0
#define PROCESS_READY 1
#define PROCESS_RUNNING 2
#define PROCESS_BLOCKED 3

struct process {
    uint32_t pid;
    uint32_t state;
    uint64_t *page_table;
    uint64_t kernel_stack;
    uint64_t instruction_pointer;
};

static struct process processes[PROCESS_MAX];
static uint32_t next_pid = 1;
static uint32_t current_pid;

void process_init(void)
{
    for (unsigned int i = 0; i < PROCESS_MAX; ++i) {
        processes[i].pid = 0;
        processes[i].state = PROCESS_UNUSED;
        processes[i].page_table = 0;
        processes[i].kernel_stack = 0;
        processes[i].instruction_pointer = 0;
    }

    processes[0].pid = next_pid++;
    processes[0].state = PROCESS_RUNNING;
    current_pid = processes[0].pid;
}

struct process *process_create(void)
{
    for (unsigned int i = 1; i < PROCESS_MAX; ++i) {
        if (processes[i].state == PROCESS_UNUSED) {
            processes[i].pid = next_pid++;
            processes[i].state = PROCESS_READY;
            return &processes[i];
        }
    }
    return 0;
}

void process_destroy(struct process *process)
{
    if (!process || process == &processes[0])
        return;
    process->state = PROCESS_UNUSED;
    process->pid = 0;
}

void scheduler_tick(void)
{
    /* Round-robin selection will be added when context switching is ready. */
    (void)current_pid;
}

uint32_t scheduler_current_pid(void)
{
    return current_pid;
}
