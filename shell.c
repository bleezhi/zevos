/* ZevOS command shell */
#include <stdint.h>
#include "vfs.h"
#define SHELL_MAX 128
static char command[SHELL_MAX]; static unsigned int command_length; static char cwd[64]="/";
void terminal_putchar(char c); void terminal_puts(const char*s); void terminal_backspace(void); void terminal_clear(void);
void installer_start(void);
struct process; extern struct process *process_create_from_path(const char *path); extern void process_launch_user(struct process *process);
static int string_equals(const char*a,const char*b){while(*a&&*b&&*a==*b){++a;++b;}return *a==0&&*b==0;} static int starts_with(const char*s,const char*p){while(*p){if(*s++!=*p++)return 0;}return 1;} static unsigned int length(const char*s){unsigned int n=0;while(s[n])++n;return n;} static void copy_string(char*d,const char*s,unsigned int m){unsigned int i=0;while(s[i]&&i+1<m){d[i]=s[i];++i;}d[i]=0;}
static void absolute_path(const char*path,char*out,unsigned int max){if(!path||!*path){copy_string(out,cwd,max);return;}if(path[0]=='/'){copy_string(out,path,max);return;}if(string_equals(cwd,"/")){out[0]='/';copy_string(out+1,path,max-1);}else{copy_string(out,cwd,max);unsigned int n=length(out);if(n+1<max){out[n++]='/';out[n]=0;}copy_string(out+n,path,max-n);}}
static void prompt(void){terminal_puts("zev@ZevOS:");terminal_puts(cwd);terminal_puts("# ");} static void list_emit(const char*n,int t){terminal_puts(n);if(t==VFS_DIR)terminal_putchar('/');terminal_puts("  ");}
static void execute_command(void){command[command_length]=0;terminal_putchar('\n');
if(command_length==0){prompt();}
else if(string_equals(command,"help")){terminal_puts("Commands:\n  help  clear  echo  pwd  ls  cd  cat  touch  mkdir  rm  cp  mv\n  whoami  id  uname  hostname  true  false  install  exec /bin/PROGRAM\n  about  ver\n");prompt();}
else if(string_equals(command,"clear")){terminal_clear();prompt();}
else if(string_equals(command,"install")){installer_start();}
else if(string_equals(command,"pwd")){terminal_puts(cwd);terminal_putchar('\n');prompt();}
else if(string_equals(command,"ls")){vfs_list(cwd,list_emit);terminal_putchar('\n');prompt();}
else if(starts_with(command,"ls ")){char path[128];absolute_path(command+3,path,sizeof(path));if(vfs_list(path,list_emit)!=0)terminal_puts("ls: no such directory\n");else terminal_putchar('\n');prompt();}
else if(string_equals(command,"cd /")){copy_string(cwd,"/",sizeof(cwd));prompt();}
else if(string_equals(command,"cd ..")){if(!string_equals(cwd,"/")){unsigned int n=length(cwd);while(n>1&&cwd[n-1]!='/')--n;if(n>1)--n;cwd[n]=0;if(n==0)copy_string(cwd,"/",sizeof(cwd));}prompt();}
else if(starts_with(command,"cd ")){char path[128];absolute_path(command+3,path,sizeof(path));if(vfs_is_dir(path))copy_string(cwd,path,sizeof(cwd));else terminal_puts("cd: no such directory\n");prompt();}
else if(string_equals(command,"whoami")){terminal_puts("zev\n");prompt();}
else if(string_equals(command,"id")){terminal_puts("uid=0(zev) gid=0(zev) groups=0(zev)\n");prompt();}
else if(string_equals(command,"uname")){terminal_puts("ZevOS\n");prompt();}
else if(string_equals(command,"uname -a")){terminal_puts("ZevOS ZevOS 0.1 x86_64 ZevOS\n");prompt();}
else if(string_equals(command,"hostname")){terminal_puts("ZevOS\n");prompt();}
else if(string_equals(command,"true")){prompt();}
else if(string_equals(command,"false")){terminal_puts("false: command returned failure\n");prompt();}
else if(starts_with(command,"cat ")){char path[128],buffer[512];absolute_path(command+4,path,sizeof(path));int n=vfs_read(path,buffer,sizeof(buffer));if(n<0)terminal_puts("cat: no such file\n");else terminal_puts(buffer);prompt();}
else if(starts_with(command,"touch ")){char path[128];absolute_path(command+6,path,sizeof(path));if(vfs_touch(path)!=0)terminal_puts("touch: cannot create file\n");prompt();}
else if(starts_with(command,"mkdir ")){char path[128];absolute_path(command+6,path,sizeof(path));if(vfs_mkdir(path)!=0)terminal_puts("mkdir: cannot create directory\n");prompt();}
else if(starts_with(command,"rm ")){char path[128];absolute_path(command+3,path,sizeof(path));int r=vfs_remove(path);if(r==-1)terminal_puts("rm: no such file or directory\n");else if(r==-2)terminal_puts("rm: directory not empty\n");prompt();}
else if(starts_with(command,"cp ")){char args[120],src[128],dst[128];copy_string(args,command+3,sizeof(args));char*space=0;for(unsigned int i=0;args[i];++i)if(args[i]==' '){space=&args[i];break;}if(!space)terminal_puts("cp: missing destination\n");else{*space=0;absolute_path(args,src,sizeof(src));absolute_path(space+1,dst,sizeof(dst));if(vfs_copy(src,dst)!=0)terminal_puts("cp: copy failed\n");}prompt();}
else if(starts_with(command,"mv ")){char args[120],src[128],dst[128];copy_string(args,command+3,sizeof(args));char*space=0;for(unsigned int i=0;args[i];++i)if(args[i]==' '){space=&args[i];break;}if(!space)terminal_puts("mv: missing destination\n");else{*space=0;absolute_path(args,src,sizeof(src));absolute_path(space+1,dst,sizeof(dst));if(vfs_copy(src,dst)!=0||vfs_remove(src)!=0)terminal_puts("mv: move failed\n");}prompt();}
else if(starts_with(command,"echo ")){char*redirect=0;for(unsigned int i=5;command[i];++i)if(command[i]=='>'&&(i==5||command[i-1]==' ')){redirect=&command[i];break;}if(redirect){*redirect=0;while(*(redirect+1)==' ')++redirect;char path[128];absolute_path(redirect+1,path,sizeof(path));if(!vfs_exists(path))vfs_touch(path);if(vfs_write(path,command+5,length(command+5))<0)terminal_puts("echo: write failed\n");}else{terminal_puts(command+5);terminal_putchar('\n');}prompt();}
else if(starts_with(command,"exec ")){char path[128];absolute_path(command+5,path,sizeof(path));struct process*p=process_create_from_path(path);if(!p)terminal_puts("exec: cannot load ELF\n");else{terminal_puts("exec: launching ");terminal_puts(path);terminal_puts("\n");process_launch_user(p);}}
else if(string_equals(command,"about")){terminal_puts("ZevOS - a tiny x86_64 hobby operating system.\nBuilt from scratch with C + Assembly.\n");prompt();}
else if(string_equals(command,"ver")){terminal_puts("ZevOS v0.1\n");prompt();}
else{terminal_puts("command not found: ");terminal_puts(command);terminal_putchar('\n');prompt();}command_length=0;}
void shell_init(void){command_length=0;copy_string(cwd,"/",sizeof(cwd));terminal_puts("ZevOS v0.1\nWelcome to the ZevOS shell.\n");terminal_puts("As this only has the 'zev (root, uid 0)' account, you are automatically logged into it.\n");terminal_puts("Type 'help' for commands.\n\n");prompt();}
void shell_input(char c){if(c=='\n'){execute_command();return;}if(c=='\b'){if(command_length>0){--command_length;terminal_backspace();}return;}if(c>=32&&c<=126&&command_length<SHELL_MAX-1){command[command_length++]=c;terminal_putchar(c);}}
