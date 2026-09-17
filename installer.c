/* ZevOS Stage 0 installer: Debian-style selectable text UI. */
#include <stdint.h>
#include "vfs.h"
#include "zevfs.h"

void terminal_clear(void); void terminal_puts(const char *s); void terminal_set_color(uint8_t color); void shell_init(void);
static int active, installed, formatted, selected_format=1, selected_copy=1, selected_boot=0;

static void draw(void){
    terminal_set_color(0x5F); terminal_clear();
    terminal_puts("\n                         ZevOS INSTALLER - STAGE 0\n\n");
    terminal_puts("                         Target: HDA0 (IDE primary master)\n\n");
    terminal_puts("                         Installation options:\n");
    terminal_puts(selected_format ? "                         [*] 1. Format HDA0 as ZevFS\n" : "                         [ ] 1. Format HDA0 as ZevFS\n");
    terminal_puts(selected_copy ? "                         [*] 2. Install /bin /etc /usr /zev\n" : "                         [ ] 2. Install /bin /etc /usr /zev\n");
    terminal_puts(selected_boot ? "                         [*] 3. Install HDA boot support\n" : "                         [ ] 3. Install HDA boot support (not ready)\n");
    terminal_puts("\n                         1-3 toggle   I install   Q quit\n\n");
    if(formatted) terminal_puts("                         [OK] ZevFS formatted on HDA0\n");
    if(installed) terminal_puts("                         [OK] ZevOS files written to HDA0\n");
    if(!selected_boot) terminal_puts("                         HDA boot: pending bootloader integration\n");
    terminal_puts("\n                         Choose your options, then press I.\n");
}

static int put(const char *path){
    uint8_t data[512]; int n=vfs_read_binary(path,data,sizeof(data)); if(n<0)return -1; return zevfs_write_file(path,data,(uint32_t)n);
}
static int install_files(void){
    const char *files[]={"/etc/hostname","/etc/motd","/zev/release","/bin/echo","/bin/cat","/bin/ls","/bin/pwd","/bin/true","/bin/false","/bin/zinit","/bin/dsplayed"};
    for(unsigned int i=0;i<sizeof(files)/sizeof(files[0]);i++)if(put(files[i]))return -1;
    return 0;
}
static void do_install(void){
    if(selected_format){if(zevfs_format()){terminal_puts("\n                         ERROR: HDA0 format failed.\n");return;}formatted=1;}
    else if(!zevfs_ready()&&zevfs_mount()){terminal_puts("\n                         ERROR: HDA0 has no ZevFS filesystem.\n");return;}
    if(selected_copy){if(install_files()){terminal_puts("\n                         ERROR: could not write ZevOS files.\n");return;}installed=1;}
    draw();
}
void installer_start(void){active=1;installed=0;formatted=0;selected_format=1;selected_copy=1;selected_boot=0;draw();}
int installer_active(void){return active;}
void installer_input(char c){
    if(!active)return;
    if(c=='1'){selected_format=!selected_format;draw();return;}
    if(c=='2'){selected_copy=!selected_copy;draw();return;}
    if(c=='3'){selected_boot=!selected_boot;draw();return;}
    if(c=='i'||c=='I'){do_install();return;}
    if(c=='q'||c=='Q'){active=0;terminal_set_color(0x0F);terminal_clear();shell_init();}
}
