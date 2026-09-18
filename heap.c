/* ZevOS small kernel heap.
 * First-fit blocks live inside pages obtained from the PMM. Freed blocks are
 * reused and adjacent free blocks are coalesced. Allocations are 16-byte aligned.
 */
#include <stdint.h>
#define PAGE_SIZE 4096ULL
#define HEAP_PAGES 1024
struct block { uint64_t size; uint8_t free; struct block *next; };
static uint8_t *pages[HEAP_PAGES];
static unsigned int page_count;
extern void *page_alloc(void);
static uint64_t align16(uint64_t n){return(n+15)&~15ULL;}
static struct block *first_block(uint8_t *p){struct block*b=(struct block*)p;b->size=PAGE_SIZE-sizeof(*b);b->free=1;b->next=0;return b;}
static void split(struct block*b,uint64_t need){
 if(b->size<=need+sizeof(struct block)+16)return;
 struct block*n=(struct block*)((uint8_t*)(b+1)+need);n->size=b->size-need-sizeof(*n);n->free=1;n->next=b->next;b->size=need;b->next=n;
}
void *kmalloc(uint64_t size){
 if(size==0)return 0;size=align16(size);if(size>PAGE_SIZE-sizeof(struct block))return 0;
 for(unsigned int p=0;p<page_count;++p)for(struct block*b=first_block(pages[p]);b; b=b->next)
   if(b->free&&b->size>=size){split(b,size);b->free=0;return(void*)(b+1);}
 if(page_count>=HEAP_PAGES)return 0;uint8_t*p=(uint8_t*)page_alloc();if(!p)return 0;pages[page_count++]=p;
 struct block*b=first_block(p);split(b,size);b->free=0;return(void*)(b+1);
}
void kfree(void *ptr){
 if(!ptr)return;
 for(unsigned int p=0;p<page_count;++p){
  uint8_t*base=pages[p];if((uint8_t*)ptr<=base||(uint8_t*)ptr>=base+PAGE_SIZE)continue;
  struct block*b=first_block(base);
  while(b){if((void*)(b+1)==ptr){b->free=1;if(b->next&&b->next->free){b->size+=sizeof(struct block)+b->next->size;b->next=b->next->next;}return;}b=b->next;}
 }
}
