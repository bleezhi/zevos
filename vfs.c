/* ZevOS VFS + tiny in-memory ramfs. */
#include <stdint.h>
#include "vfs.h"
#define VFS_MAX_NODES 128
#define VFS_NAME_MAX 24
#define VFS_DATA_MAX 512
struct vfs_node { uint8_t used; uint8_t type; int16_t parent; char name[VFS_NAME_MAX]; char data[VFS_DATA_MAX]; uint32_t size; };
static struct vfs_node nodes[VFS_MAX_NODES];
static int str_eq(const char*a,const char*b){while(*a&&*b&&*a==*b){++a;++b;}return *a==0&&*b==0;}
static unsigned int str_len(const char*s){unsigned int n=0;while(s[n])++n;return n;}
static void str_copy(char*d,const char*s,unsigned int m){unsigned int i=0;while(s[i]&&i+1<m){d[i]=s[i];++i;}d[i]=0;}
static int child(int p,const char*n){for(int i=0;i<VFS_MAX_NODES;++i)if(nodes[i].used&&nodes[i].parent==p&&str_eq(nodes[i].name,n))return i;return -1;}
static int lookup(const char*path){if(!path||path[0]!='/')return -1;if(path[1]==0)return 0;int cur=0;unsigned int i=1;char part[VFS_NAME_MAX];while(path[i]){unsigned int n=0;while(path[i]=='/')++i;while(path[i]&&path[i]!='/'&&n+1<VFS_NAME_MAX)part[n++]=path[i++];part[n]=0;if(!n||str_eq(part,"."))continue;if(str_eq(part,"..")){if(nodes[cur].parent>=0)cur=nodes[cur].parent;continue;}cur=child(cur,part);if(cur<0)return -1;}return cur;}
static int split_parent(const char*path,char*parent,char*name){if(!path)return -1;unsigned int len=str_len(path);if(path[0]!='/'||len==0||len>=256)return -1;char tmp[256];for(unsigned int i=0;i<=len;++i)tmp[i]=path[i];while(len>1&&tmp[len-1]=='/')tmp[--len]=0;int slash=-1;for(unsigned int i=1;i<len;++i)if(tmp[i]=='/')slash=(int)i;if(slash<0){str_copy(parent,"/",256);str_copy(name,tmp+1,VFS_NAME_MAX);}else{tmp[slash]=0;str_copy(parent,tmp,256);str_copy(name,tmp+slash+1,VFS_NAME_MAX);}return name[0]?0:-1;}
static int create_node(const char*path,int type){char pp[256],name[VFS_NAME_MAX];if(split_parent(path,pp,name)!=0||str_eq(name,".")||str_eq(name,".."))return -1;int p=lookup(pp);if(p<0||nodes[p].type!=VFS_DIR||child(p,name)>=0)return -1;int slot=-1;for(int i=1;i<VFS_MAX_NODES;++i)if(!nodes[i].used){slot=i;break;}if(slot<0)return -1;nodes[slot].used=1;nodes[slot].type=(uint8_t)type;nodes[slot].parent=(int16_t)p;nodes[slot].size=0;nodes[slot].name[0]=0;str_copy(nodes[slot].name,name,VFS_NAME_MAX);return slot;}
void vfs_init(void){for(int i=0;i<VFS_MAX_NODES;++i)nodes[i].used=0;nodes[0].used=1;nodes[0].type=VFS_DIR;nodes[0].parent=0;nodes[0].name[0]=0;vfs_mkdir("/bin");vfs_mkdir("/dev");vfs_mkdir("/etc");vfs_mkdir("/home");vfs_mkdir("/tmp");vfs_mkdir("/usr");vfs_mkdir("/var");vfs_mkdir("/usr/bin");vfs_mkdir("/usr/lib");vfs_touch("/etc/hostname");vfs_write("/etc/hostname","ZevOS\n",6);vfs_touch("/etc/motd");vfs_write("/etc/motd","Welcome to ZevOS.\n",19);vfs_install_binaries();}
int vfs_exists(const char*p){return lookup(p)>=0;} int vfs_is_dir(const char*p){int n=lookup(p);return n>=0&&nodes[n].type==VFS_DIR;}
int vfs_mkdir(const char*p){return create_node(p,VFS_DIR)>=0?0:-1;} int vfs_touch(const char*p){return create_node(p,VFS_FILE)>=0?0:-1;}
int vfs_remove(const char*p){int n=lookup(p);if(n<=0)return -1;if(nodes[n].type==VFS_DIR)for(int i=1;i<VFS_MAX_NODES;++i)if(nodes[i].used&&nodes[i].parent==n)return -2;nodes[n].used=0;return 0;}
int vfs_read_binary(const char*p,void*b,uint64_t size){int n=lookup(p);if(n<0||nodes[n].type!=VFS_FILE||!b)return -1;if(size<nodes[n].size)return -2;for(uint64_t i=0;i<nodes[n].size;++i)((uint8_t*)b)[i]=(uint8_t)nodes[n].data[i];return (int)nodes[n].size;}
int vfs_read(const char*p,char*b,uint64_t size){int n=lookup(p);if(n<0||nodes[n].type!=VFS_FILE||!b||size==0)return -1;uint64_t c=nodes[n].size<size-1?nodes[n].size:size-1;for(uint64_t i=0;i<c;++i)b[i]=nodes[n].data[i];b[c]=0;return (int)c;}
int vfs_write_binary(const char*p,const void*d,uint64_t size){int n=lookup(p);if(n<0||nodes[n].type!=VFS_FILE||!d||size>=VFS_DATA_MAX)return -1;for(uint64_t i=0;i<size;++i)nodes[n].data[i]=(char)((const uint8_t*)d)[i];nodes[n].size=(uint32_t)size;return (int)size;}
int vfs_write(const char*p,const char*d,uint64_t size){return vfs_write_binary(p,d,size);}
int vfs_copy(const char*s,const char*d){int n=lookup(s);if(n<0||nodes[n].type!=VFS_FILE)return -1;int x=lookup(d);if(x<0){if(vfs_touch(d)!=0)return -1;x=lookup(d);}if(x<0||nodes[x].type!=VFS_FILE)return -1;return vfs_write_binary(d,nodes[n].data,nodes[n].size)<0?-1:0;}
int vfs_list(const char*p,void(*emit)(const char*,int)){int n=lookup(p);if(n<0||nodes[n].type!=VFS_DIR||!emit)return -1;for(int i=1;i<VFS_MAX_NODES;++i)if(nodes[i].used&&nodes[i].parent==n)emit(nodes[i].name,nodes[i].type);return 0;}
