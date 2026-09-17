#include <stdint.h>
#include "ata.h"

static inline void outb(uint16_t p,uint8_t v){__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}
static inline uint8_t inb(uint16_t p){uint8_t v;__asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p));return v;}
static inline void insw(uint16_t p,void *b,uint32_t n){__asm__ volatile("rep insw":"+D"(b),"+c"(n):"d"(p):"memory");}
static inline void outsw(uint16_t p,const void *b,uint32_t n){__asm__ volatile("rep outsw":"+S"(b),"+c"(n):"d"(p):"memory");}
static void io_wait(void){inb(0x3f6);}
static int wait_bsy(void){for(uint32_t i=0;i<1000000;i++)if(!(inb(0x1f7)&0x80))return 0;return -1;}
static int wait_drq(void){for(uint32_t i=0;i<1000000;i++){uint8_t s=inb(0x1f7);if(s&1)return -1;if(s&8)return 0;}return -1;}
int ata_present(void){outb(0x1f6,0xa0);io_wait();outb(0x1f7,0xec);io_wait();return inb(0x1f7)!=0;}
int ata_init(void){return ata_present()?0:-1;}
int ata_read28(uint32_t lba,uint8_t *b){if(!b||lba>0x0fffffff)return -1;if(wait_bsy())return -1;outb(0x1f2,1);outb(0x1f3,lba);outb(0x1f4,lba>>8);outb(0x1f5,lba>>16);outb(0x1f6,0xe0|((lba>>24)&15));outb(0x1f7,0x20);if(wait_drq())return -1;insw(0x1f0,b,256);return 0;}
int ata_write28(uint32_t lba,const uint8_t *b){if(!b||lba>0x0fffffff)return -1;if(wait_bsy())return -1;outb(0x1f2,1);outb(0x1f3,lba);outb(0x1f4,lba>>8);outb(0x1f5,lba>>16);outb(0x1f6,0xe0|((lba>>24)&15));outb(0x1f7,0x30);if(wait_drq())return -1;outsw(0x1f0,b,256);outb(0x1f7,0xe7);return wait_bsy();}
