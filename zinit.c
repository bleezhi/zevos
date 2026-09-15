/* zinit - ZevOS service/init foundation.
 * For now zinit lives in the kernel. It will become PID 1 once user mode,
 * ELF loading, and system calls are available.
 */

#include <stdint.h>

void terminal_puts(const char *s);

#define ZINIT_MAX_SERVICES 16
#define SERVICE_STOPPED 0
#define SERVICE_RUNNING 1

struct zinit_service {
    const char *name;
    uint32_t state;
};

static struct zinit_service services[ZINIT_MAX_SERVICES];
static unsigned int service_count;

static void zinit_register(const char *name)
{
    if (service_count >= ZINIT_MAX_SERVICES)
        return;
    services[service_count].name = name;
    services[service_count].state = SERVICE_STOPPED;
    ++service_count;
}

static void zinit_start(unsigned int index)
{
    if (index < service_count)
        services[index].state = SERVICE_RUNNING;
}

void zinit_init(void)
{
    service_count = 0;

    terminal_puts("zinit: starting service manager...\n");

    zinit_register("console");
    zinit_register("shell");

    zinit_start(0);
    zinit_start(1);

    terminal_puts("zinit: services ready\n");
    terminal_puts("zinit: done starting fully!\n");
}

unsigned int zinit_service_count(void)
{
    return service_count;
}
