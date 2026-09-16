/* ZevOS process and scheduler foundation. */

#include <stdint.h>

#define PROCESS_MAX 64
#define PROCESS_UNUSED 0
#define PROCESS_READY 1
#define PROCESS_RUNNING 2
#define PROCESS_BLOCKED 3

#define USER_STACK_TOP 0x701000ULL
#define USER_STACK_PAGE 0x700000ULL

struct process {
    uint32_t pid;
    uint32_t state;
    uint64_t *page_table;
    uint64_t kernel_stack;
    uint64_t kernel_rsp;
    uint64_t instruction_pointer;
    uint64_t user_rip;
    uint64_t user_rsp;
    uint64_t user_stack_phys;
};

static struct process processes[PROCESS_MAX];
static uint32_t next_pid = 1;
static uint32_t current_index;

extern void process_switch(uint64_t *old_rsp, uint64_t new_rsp);
extern uint64_t vmm_create_address_space(void);
extern int vmm_map_user_page(uint64_t cr3, uint64_t virtual_address, uint64_t physical_address);
extern void *page_alloc(void);
extern void page_free(void *address);

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
        processes[i].user_stack_phys = 0;
    }

    processes[0].pid = next_pid++;
    processes[0].state = PROCESS_RUNNING;
    current_index = 0;
}

struct process *process_create(void)
{
    for (unsigned int i = 1; i < PROCESS_MAX; ++i) {
        if (processes[i].state != PROCESS_UNUSED)
            continue;

        uint64_t cr3 = vmm_create_address_space();
        if (!cr3)
            return 0;

        void *stack_page = page_alloc();
        if (!stack_page)
            return 0;

        if (vmm_map_user_page(cr3, USER_STACK_PAGE, (uint64_t)stack_page) != 0) {
            page_free(stack_page);
            return 0;
        }

        processes[i].pid = next_pid++;
        processes[i].state = PROCESS_READY;
        processes[i].page_table = (uint64_t *)cr3;
        processes[i].kernel_stack = 0;
        processes[i].kernel_rsp = 0;
        processes[i].instruction_pointer = 0;
        processes[i].user_rip = 0;
        processes[i].user_rsp = USER_STACK_TOP;
        processes[i].user_stack_phys = (uint64_t)stack_page;
        return &processes[i];
    }
    return 0;
}

void process_destroy(struct process *process)
{
    if (!process || process == &processes[0])
        return;

    if (process->user_stack_phys)
        page_free((void *)process->user_stack_phys);

    process->state = PROCESS_UNUSED;
    process->pid = 0;
    process->page_table = 0;
    process->kernel_stack = 0;
    process->kernel_rsp = 0;
    process->instruction_pointer = 0;
    process->user_rip = 0;
    process->user_rsp = 0;
    process->user_stack_phys = 0;
}

/* Select the next runnable process. A process with no prepared kernel stack
 * is left alone until the context/ELF layer supplies one. */
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
