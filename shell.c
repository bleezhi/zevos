/* ZevOS command shell */

#define SHELL_MAX 128

void terminal_putchar(char c);
void terminal_puts(const char *s);
void terminal_backspace(void);
void terminal_clear(void);

static char command[SHELL_MAX];
static unsigned int command_length;

static int string_equals(const char *a, const char *b)
{
    while (*a && *b && *a == *b) {
        ++a;
        ++b;
    }
    return *a == '\0' && *b == '\0';
}

static int starts_with(const char *s, const char *prefix)
{
    while (*prefix) {
        if (*s++ != *prefix++)
            return 0;
    }
    return 1;
}

static void prompt(void)
{
    terminal_puts("zev@ZevOS:~$ ");
}

static void execute_command(void)
{
    command[command_length] = '\0';

    terminal_putchar('\n');

    if (command_length == 0) {
        prompt();
    } else if (string_equals(command, "help")) {
        terminal_puts("Commands:\n");
        terminal_puts("  help       Show this help\n");
        terminal_puts("  clear      Clear the screen\n");
        terminal_puts("  echo TEXT  Print TEXT\n");
        terminal_puts("  about      About ZevOS\n");
        terminal_puts("  ver        Show version\n");
        prompt();
    } else if (string_equals(command, "clear")) {
        terminal_clear();
        prompt();
    } else if (string_equals(command, "about")) {
        terminal_puts("ZevOS - a tiny x86_64 hobby operating system.\n");
        terminal_puts("Built from scratch with C + Assembly.\n");
        prompt();
    } else if (string_equals(command, "ver")) {
        terminal_puts("ZevOS v0.1\n");
        prompt();
    } else if (starts_with(command, "echo ")) {
        terminal_puts(command + 5);
        terminal_putchar('\n');
        prompt();
    } else {
        terminal_puts("command not found: ");
        terminal_puts(command);
        terminal_putchar('\n');
        prompt();
    }

    command_length = 0;
}

void shell_init(void)
{
    command_length = 0;
    terminal_puts("ZevOS v0.1\n");
    terminal_puts("Welcome to the ZevOS shell.\n");
    terminal_puts("Type 'help' for commands.\n\n");
    prompt();
}

void shell_input(char c)
{
    if (c == '\n') {
        execute_command();
        return;
    }

    if (c == '\b') {
        if (command_length > 0) {
            --command_length;
            terminal_backspace();
        }
        return;
    }

    if (c >= 32 && c <= 126 && command_length < SHELL_MAX - 1) {
        command[command_length++] = c;
        terminal_putchar(c);
    }
}
