/* ZevOS terminal backspace helper */

void terminal_putchar(char c);

void terminal_backspace(void)
{
    terminal_putchar('\b');
}
