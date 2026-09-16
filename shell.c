/* ZevOS command shell */

#define SHELL_MAX 128

void terminal_putchar(char c);
void terminal_puts(const char *s);
void terminal_backspace(void);
void terminal_clear(void);

static char command[SHELL_MAX];
static unsigned int command_length;
static char cwd[64] = "/";

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
    terminal_puts("zev@ZevOS:");
    terminal_puts(cwd);
    terminal_puts("# ");
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
        terminal_puts("  pwd        Print working directory\n");
        terminal_puts("  ls         List directory\n");
        terminal_puts("  cd DIR     Change directory\n");
        terminal_puts("  whoami     Print current user\n");
        terminal_puts("  id         Print user and group IDs\n");
        terminal_puts("  uname      Print system information\n");
        terminal_puts("  hostname   Print system hostname\n");
        terminal_puts("  true       Return success\n");
        terminal_puts("  false      Return failure\n");
        terminal_puts("  about      About ZevOS\n");
        terminal_puts("  ver        Show version\n");
        prompt();
    } else if (string_equals(command, "clear")) {
        terminal_clear();
        prompt();
    } else if (string_equals(command, "pwd")) {
        terminal_puts(cwd);
        terminal_putchar('\n');
        prompt();
    } else if (string_equals(command, "ls")) {
        terminal_puts("bin  dev  etc  home  tmp  usr  var\n");
        prompt();
    } else if (string_equals(command, "ls /")) {
        terminal_puts("bin  dev  etc  home  tmp  usr  var\n");
        prompt();
    } else if (string_equals(command, "cd /")) {
        cwd[0] = '/';
        cwd[1] = '\0';
        prompt();
    } else if (string_equals(command, "cd ..")) {
        cwd[0] = '/';
        cwd[1] = '\0';
        prompt();
    } else if (starts_with(command, "cd ")) {
        const char *dir = command + 3;
        if (string_equals(dir, "bin") || string_equals(dir, "dev") ||
            string_equals(dir, "etc") || string_equals(dir, "home") ||
            string_equals(dir, "tmp") || string_equals(dir, "usr") ||
            string_equals(dir, "var")) {
            if (cwd[1] == '\0') {
                unsigned int i = 0;
                cwd[0] = '/';
                while (dir[i] && i < 60) {
                    cwd[i + 1] = dir[i];
                    ++i;
                }
                cwd[i + 1] = '\0';
            } else {
                terminal_puts("cd: nested directories are not available yet\n");
            }
        } else {
            terminal_puts("cd: no such directory\n");
        }
        prompt();
    } else if (string_equals(command, "whoami")) {
        terminal_puts("zev\n");
        prompt();
    } else if (string_equals(command, "id")) {
        terminal_puts("uid=0(zev) gid=0(zev) groups=0(zev)\n");
        prompt();
    } else if (string_equals(command, "uname")) {
        terminal_puts("ZevOS\n");
        prompt();
    } else if (string_equals(command, "uname -a")) {
        terminal_puts("ZevOS ZevOS 0.1 x86_64 ZevOS\n");
        prompt();
    } else if (string_equals(command, "hostname")) {
        terminal_puts("ZevOS\n");
        prompt();
    } else if (string_equals(command, "true")) {
        prompt();
    } else if (string_equals(command, "false")) {
        terminal_puts("false: command returned failure\n");
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
    cwd[0] = '/';
    cwd[1] = '\0';
    terminal_puts("ZevOS v0.1\n");
    terminal_puts("Welcome to the ZevOS shell.\n");
    terminal_puts("As this only has the 'zev (root, uid 0)' account, you are automatically logged into it.\n");
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
