/* ZevOS process and scheduler foundation. */

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
    uint64_t kernel_rsp;
    uint64_t instruction_pointer;
    uint64_t user_rip;
    uint64_t user_rsp;
};

static struct process processes[PROCESS_MAX];
static uint32_t next_pid = 1;
static uint32_t current_index;

extern void process_switch(uint64_t *old_rsp, uint64_t new_rsp);

static unsigned int next_ready(unsigned int start)
{
    for (unsigned int n = 1; n <= PROCESS_MAX; ++n) {
        unsigned int i = (start + n) % PROCESS_MAX;
        if ((processes[i].state == PROCESS_READY ||
             processes[i].state == PROCESS_RUNNING) &&
            processes[i].kernel_rsp != 0)
            return i;
    }
    return start;
}

void process_init(void)
{
    for (unsigned int i = 0; i < PROCESS_MAX; ++i) {
        processes[i].pid = 0;
        processes[i].state = PROCESS_UNUSED;
        processes[i].page_table = 0;
        processes[i].kernel_stack = 0;
        processes[i].kernel_rsp = 0;
        processes[i].instruction_pointer = 0;
        processes[i].user_rip = 0;
        processes[i].user_rsp = 0;
    }

    processes[0].pid = next_pid++;
    processes[0].state = PROCESS_RUNNING;
    current_index = 0;
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

/* Select the next runnable process. A process with no prepared kernel stack
 * is left alone until the process/ELF layer supplies one. */
void scheduler_tick(void)
{
    unsigned int old = current_index;
    unsigned int next = next_ready(old);

    if (next == old)
        return;

    processes[old].state = PROCESS_READY;
    processes[next].state = PROCESS_RUNNING;
    current_index = next;
    process_switch(&processes[old].kernel_rsp, processes[next].kernel_rsp);
}

uint32_t scheduler_current_pid(void)
{
    return processes[current_index].pid;
}

uint32_t process_count(void)
{
    uint32_t count = 0;
    for (unsigned int i = 0; i < PROCESS_MAX; ++i)
        if (processes[i].state != PROCESS_UNUSED)
            ++count;
    return count;
}
