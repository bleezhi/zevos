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
    uint64_t user_code_phys;
};

static struct process processes[PROCESS_MAX];
static uint32_t next_pid = 1;
static uint32_t current_index;
static uint32_t first_user_error;

extern void process_switch(uint64_t *old_rsp, uint64_t new_rsp);
extern uint64_t vmm_create_address_space(void);
extern int vmm_map_user_page(uint64_t cr3, uint64_t virtual_address, uint64_t physical_address);
extern void *page_alloc(void);
extern void page_free(void *address);
extern void user_launch(uint64_t cr3, uint64_t entry, uint64_t stack);
extern char user_program_start[];
extern char user_program_end[];
extern int elf_load_image(uint64_t cr3, const void *image, uint64_t image_size,
                          uint64_t *entry_out, uint64_t *first_page_out);

void process_destroy(struct process *process);

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
        processes[i].user_code_phys = 0;
    }

    processes[0].pid = next_pid++;
    processes[0].state = PROCESS_RUNNING;
    current_index = 0;
    first_user_error = 0;
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
        if (!stack_page) {
            page_free((void *)cr3);
            return 0;
        }

        if (vmm_map_user_page(cr3, USER_STACK_PAGE, (uint64_t)stack_page) != 0) {
            page_free(stack_page);
            page_free((void *)cr3);
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
        processes[i].user_code_phys = 0;
        return &processes[i];
    }
    return 0;
}

/* Create the first userspace process from an ELF64 image. */
struct process *process_create_first_user(void)
{
    first_user_error = 0;

    struct process *process = process_create();
    if (!process) {
        first_user_error = 1;
        return 0;
    }

    uint64_t image_size = (uint64_t)(user_program_end - user_program_start);
    uint64_t entry = 0;
    uint64_t code_page = 0;
    int elf_result = elf_load_image((uint64_t)process->page_table,
                                    user_program_start, image_size,
                                    &entry, &code_page);
    if (elf_result != 0) {
        first_user_error = (uint32_t)(100 - elf_result);
        process_destroy(process);
        return 0;
    }

    process->user_code_phys = code_page;
    process->user_rip = entry;
    process->instruction_pointer = entry;
    return process;
}

uint32_t process_first_user_error(void)
{
    return first_user_error;
}

void process_launch_user(struct process *process)
{
    if (!process || !process->page_table || !process->user_rip || !process->user_rsp)
        return;

    current_index = (unsigned int)(process - processes);
    process->state = PROCESS_RUNNING;
    user_launch((uint64_t)process->page_table,
                process->user_rip,
                process->user_rsp);

    __builtin_unreachable();
}

void process_destroy(struct process *process)
{
    if (!process || process == &processes[0])
        return;

    if (process->user_stack_phys)
        page_free((void *)process->user_stack_phys);
    if (process->user_code_phys)
        page_free((void *)process->user_code_phys);

    process->state = PROCESS_UNUSED;
    process->pid = 0;
    process->page_table = 0;
    process->kernel_stack = 0;
    process->kernel_rsp = 0;
    process->instruction_pointer = 0;
    process->user_rip = 0;
    process->user_rsp = 0;
    process->user_stack_phys = 0;
    process->user_code_phys = 0;
}

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
