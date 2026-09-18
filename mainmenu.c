/* ZevOS installer/main menu. Press M from the shell to open it. */
#include <stdint.h>
void terminal_clear(void); void terminal_puts(const char *s); void terminal_set_color(uint8_t color); void shell_init(void); void installer_start(void);
static int active;
void mainmenu_start(void){active=1;terminal_set_color(0x1F);terminal_clear();terminal_puts("\n                 +--------------------------------------+\n                 |       [ ? ] ZevOS main menu         |\n                 +--------------------------------------+\n\n                 Choose the next step:\n\n                   1  Install ZevOS\n                   2  Open shell\n                   3  Reboot\n                   4  Return to shell\n\n                 Press a number, or M to close.\n");}
int mainmenu_active(void){return active;}
static void reboot(void){terminal_puts("\n                 Rebooting...\n");__asm__ volatile("cli");__asm__ volatile("outb %0,%1"::"a"((uint8_t)0xFE),"Nd"((uint16_t)0x64));for(;;)__asm__ volatile("hlt");}
void mainmenu_input(char c){if(!active)return;if(c=='1'){active=0;terminal_set_color(0x0F);installer_start();return;}if(c=='2'||c=='4'||c=='m'||c=='M'){active=0;terminal_set_color(0x0F);terminal_clear();shell_init();return;}if(c=='3')reboot();}
