#include <stdint.h>
#include "ata.h"
#include "zevfs.h"

#define ZEVFS_MAGIC 0x5a455646u
#define ZEVFS_SLOTS 24
#define SLOT_SECTORS 5
/* LBA 0 = ZevOS boot sector, LBA 1 = HDA boot header. */
#define META_LBA 64
#define DATA_LBA (META_LBA + ZEVFS_SLOTS*SLOT_SECTORS)

struct zevfs_meta { uint32_t magic; uint32_t size; char path[64]; };
static int ready;
static void zero(uint8_t *p){for(int i=0;i<512;i++)p[i]=0;}
static int eq(const char*a,const char*b){while(*a&&*b&&*a==*b){a++;b++;}return *a==0&&*b==0;}
static int slot_for(const char *path,int create){uint8_t s[512];struct zevfs_meta *m=(struct zevfs_meta*)s;for(int i=0;i<ZEVFS_SLOTS;i++){if(ata_read28(META_LBA+i*SLOT_SECTORS,s))return -1;if(m->magic==ZEVFS_MAGIC&&eq(m->path,path))return i;}if(!create)return -1;for(int i=0;i<ZEVFS_SLOTS;i++){if(ata_read28(META_LBA+i*SLOT_SECTORS,s))return -1;if(m->magic==0){zero(s);m->magic=ZEVFS_MAGIC;for(int j=0;j<64;j++)m->path[j]=path[j];m->size=0;if(ata_write28(META_LBA+i*SLOT_SECTORS,s))return -1;return i;}}return -1;}
int zevfs_ready(void){return ready;}
int zevfs_format(void){uint8_t s[512];if(ata_init())return -1;for(int i=0;i<ZEVFS_SLOTS*SLOT_SECTORS;i++){zero(s);if(ata_write28(META_LBA+i,s))return -1;}ready=1;return 0;}
int zevfs_mount(void){uint8_t s[512];if(ata_init())return -1;if(ata_read28(META_LBA,s))return -1;struct zevfs_meta*m=(struct zevfs_meta*)s;if(m->magic!=ZEVFS_MAGIC)return -1;ready=1;return 0;}
int zevfs_write_file(const char *path,const void *data,uint32_t size){uint8_t s[512];if(!ready||!data||size>2048)return -1;int slot=slot_for(path,1);if(slot<0)return -1;struct zevfs_meta*m=(struct zevfs_meta*)s;if(ata_read28(META_LBA+slot*SLOT_SECTORS,s))return -1;m->size=size;if(ata_write28(META_LBA+slot*SLOT_SECTORS,s))return -1;const uint8_t*d=(const uint8_t*)data;for(int i=0;i<4;i++){zero(s);for(int j=0;j<512&&i*512+j<(int)size;j++)s[j]=d[i*512+j];if(ata_write28(DATA_LBA+slot*SLOT_SECTORS+i,s))return -1;}return 0;}
int zevfs_read_file(const char *path,void *data,uint32_t capacity){uint8_t s[512];if(!ready||!data)return -1;int slot=slot_for(path,0);if(slot<0)return -1;if(ata_read28(META_LBA+slot*SLOT_SECTORS,s))return -1;struct zevfs_meta*m=(struct zevfs_meta*)s;if(m->size>capacity)return -2;uint8_t*d=(uint8_t*)data;for(int i=0;i<4&&i*512<m->size;i++){if(ata_read28(DATA_LBA+slot*SLOT_SECTORS+i,s))return -1;for(int j=0;j<512&&i*512+j<(int)m->size;j++)d[i*512+j]=s[j];}return (int)m->size;}
