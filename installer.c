/* ZevOS Stage 0 installer: Debian-style selectable text UI. */
#include <stdint.h>
#include "vfs.h"
#include "zevfs.h"
#include "ata.h"

void terminal_clear(void); void terminal_puts(const char *s); void terminal_set_color(uint8_t color); void shell_init(void);
extern const uint8_t hda_boot_start[];
extern const uint8_t hda_boot_end[];
extern const uint8_t kernel_start[];
extern const uint8_t kernel_end[];

static int active, installed, formatted, boot_installed;
static int selected_format=1, selected_copy=1, selected_boot=1;

static void draw(void){
    terminal_set_color(0x5F); terminal_clear();
    terminal_puts("\n                         ZevOS INSTALLER - STAGE 0\n\n");
    terminal_puts("                         Target: HDA0 (IDE primary master)\n\n");
    terminal_puts("                         Installation options:\n");
    terminal_puts(selected_format ? "                         [*] 1. Format HDA0 as ZevFS\n" : "                         [ ] 1. Format HDA0 as ZevFS\n");
    terminal_puts(selected_copy ? "                         [*] 2. Install /bin /etc /usr /zev\n" : "                         [ ] 2. Install /bin /etc /usr /zev\n");
    terminal_puts(selected_boot ? "                         [*] 3. Install HDA bootloader + kernel\n" : "                         [ ] 3. Install HDA bootloader + kernel\n");
    terminal_puts("\n                         1-3 toggle   I install   R reboot   Q quit\n\n");
    if(formatted) terminal_puts("                         [OK] ZevFS formatted on HDA0\n");
    if(installed) terminal_puts("                         [OK] ZevOS files written to HDA0\n");
    if(boot_installed) terminal_puts("                         [OK] HDA bootloader + kernel installed\n");
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
static void le32(uint8_t *p,uint32_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24);}

/* The running kernel is already laid out at its final physical address (1 MiB).
   Copying that memory range makes the installed HDA kernel a bootable flat image,
   including the zeroed BSS required by the direct HDA entry path. */
static int install_hda_boot(void){
    uint8_t sector[512];
    uint32_t boot_size=(uint32_t)(hda_boot_end-hda_boot_start);
    uint32_t kernel_bytes=(uint32_t)(kernel_end-kernel_start);
    uint32_t kernel_sectors=(kernel_bytes+511u)/512u;
    if(boot_size!=512u||!kernel_sectors)return -1;
    for(int i=0;i<512;i++)sector[i]=0;
    for(uint32_t i=0;i<512;i++)sector[i]=hda_boot_start[i];
    if(ata_write28(0,sector))return -1;
    for(int i=0;i<512;i++)sector[i]=0;
    sector[0]='Z';sector[1]='B';sector[2]='O';sector[3]='T';
    le32(sector+4,kernel_sectors);
    le32(sector+8,kernel_bytes);
    le32(sector+12,(uint32_t)(kernel_end-kernel_start));
    if(ata_write28(1,sector))return -1;
    const uint8_t *src=kernel_start;
    for(uint32_t s=0;s<kernel_sectors;s++){
        for(int j=0;j<512;j++)sector[j]=(s*512u+j<kernel_bytes)?src[s*512u+j]:0;
        if(ata_write28(2+s,sector))return -1;
    }
    return 0;
}

static void do_install(void){
    if(ata_init()){terminal_puts("\n                         ERROR: HDA0 not detected.\n");return;}
    if(selected_format){if(zevfs_format()){terminal_puts("\n                         ERROR: HDA0 format failed.\n");return;}formatted=1;}
    else if(!zevfs_ready()&&zevfs_mount()){terminal_puts("\n                         ERROR: HDA0 has no ZevFS filesystem.\n");return;}
    if(selected_copy){if(install_files()){terminal_puts("\n                         ERROR: could not write ZevOS files.\n");return;}installed=1;}
    if(selected_boot){if(install_hda_boot()){terminal_puts("\n                         ERROR: HDA boot installation failed.\n");return;}boot_installed=1;}
    draw();
}

static void reboot_machine(void){
    terminal_puts("\n                         Rebooting... remove/eject the ISO or boot HDA first.\n");
    for(volatile uint32_t i=0;i<1000000;i++)__asm__ volatile("pause");
    for(volatile uint32_t i=0;i<10000;i++)__asm__ volatile("inb %%dx,%%al"::"d"(0x64):"al");
    __asm__ volatile("outb %0,%1"::"a"((uint8_t)0xFE),"Nd"((uint16_t)0x64));
    for(;;)__asm__ volatile("hlt");
}

void installer_start(void){active=1;installed=0;formatted=0;boot_installed=0;selected_format=1;selected_copy=1;selected_boot=1;draw();}
int installer_active(void){return active;}
void installer_input(char c){
    if(!active)return;
    if(c=='1'){selected_format=!selected_format;draw();return;}
    if(c=='2'){selected_copy=!selected_copy;draw();return;}
    if(c=='3'){selected_boot=!selected_boot;draw();return;}
    if(c=='i'||c=='I'){do_install();return;}
    if((c=='r'||c=='R')&&boot_installed){reboot_machine();return;}
    if(c=='q'||c=='Q'){active=0;terminal_set_color(0x0F);terminal_clear();shell_init();}
}
