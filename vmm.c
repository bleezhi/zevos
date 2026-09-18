/* ZevOS virtual-memory subsystem. */
#include <stdint.h>
#define PAGE_SIZE 4096ULL
#define PAGE_PRESENT 1ULL
#define PAGE_WRITE 2ULL
#define PAGE_USER 4ULL
#define PAGE_HUGE 128ULL
#define PAGE_TABLE_ENTRIES 512
#define USER_MIN 0x400000ULL
#define USER_MAX 0x800000ULL
extern void *page_alloc(void);
extern void page_free(void *address);
static uint64_t *kernel_pml4;
static void zero_page(uint64_t *page){for(unsigned int i=0;i<PAGE_TABLE_ENTRIES;++i)page[i]=0;}
static uint64_t *new_table(void){uint64_t *p=(uint64_t*)page_alloc();if(!p)return 0;zero_page(p);return p;}
static void free_table(uint64_t *p){if(p)page_free(p);}
static uint64_t *build_identity_space(void){
 uint64_t *pml4=new_table(),*pdpt=new_table(),*pd=new_table();
 if(!pml4||!pdpt||!pd){free_table(pml4);free_table(pdpt);free_table(pd);return 0;}
 pml4[0]=(uint64_t)pdpt|PAGE_PRESENT|PAGE_WRITE;
 pdpt[0]=(uint64_t)pd|PAGE_PRESENT|PAGE_WRITE;
 for(unsigned int i=0;i<PAGE_TABLE_ENTRIES;++i)pd[i]=((uint64_t)i<<21)|PAGE_PRESENT|PAGE_WRITE|PAGE_HUGE;
 return pml4;
}
void vmm_init(void){kernel_pml4=build_identity_space();if(kernel_pml4)__asm__ volatile("mov %0,%%cr3"::"r"((uint64_t)kernel_pml4):"memory");}
uint64_t vmm_kernel_cr3(void){return(uint64_t)kernel_pml4;}
uint64_t vmm_create_address_space(void){return(uint64_t)build_identity_space();}
static uint64_t *split_huge_page(uint64_t *pd,unsigned int i){
 uint64_t e=pd[i];if(!(e&PAGE_PRESENT)||(e&PAGE_HUGE))return 0;uint64_t *pt=new_table();if(!pt)return 0;
 uint64_t base=e&~0x1FFFFFULL,flags=e&0xFFFULL;flags&=~PAGE_HUGE;
 for(unsigned int n=0;n<PAGE_TABLE_ENTRIES;++n)pt[n]=(base+(uint64_t)n*PAGE_SIZE)|flags;
 pd[i]=(uint64_t)pt|PAGE_PRESENT|PAGE_WRITE|PAGE_USER;return pt;
}
int vmm_map_user_page(uint64_t cr3,uint64_t va,uint64_t pa){
 if(!cr3||(va&(PAGE_SIZE-1))||(pa&(PAGE_SIZE-1))||va<USER_MIN||va>=USER_MAX)return -1;
 uint64_t *pml4=(uint64_t*)cr3;unsigned int a=(va>>39)&511,b=(va>>30)&511,c=(va>>21)&511,d=(va>>12)&511;
 uint64_t *pdpt,*pd,*pt;
 if(!(pml4[a]&PAGE_PRESENT)){pdpt=new_table();if(!pdpt)return -1;pml4[a]=(uint64_t)pdpt|PAGE_PRESENT|PAGE_WRITE|PAGE_USER;}
 else{pdpt=(uint64_t*)(pml4[a]&~0xFFFULL);pml4[a]|=PAGE_USER;}
 if(!(pdpt[b]&PAGE_PRESENT)){pd=new_table();if(!pd)return -1;pdpt[b]=(uint64_t)pd|PAGE_PRESENT|PAGE_WRITE|PAGE_USER;}
 else{if(pdpt[b]&PAGE_HUGE)return -1;pd=(uint64_t*)(pdpt[b]&~0xFFFULL);pdpt[b]|=PAGE_USER;}
 if(!(pd[c]&PAGE_PRESENT)){pt=new_table();if(!pt)return -1;pd[c]=(uint64_t)pt|PAGE_PRESENT|PAGE_WRITE|PAGE_USER;}
 else if(pd[c]&PAGE_HUGE){pt=split_huge_page(pd,c);if(!pt)return -1;}
 else{pt=(uint64_t*)(pd[c]&~0xFFFULL);pd[c]|=PAGE_USER;}
 if((pt[d]&PAGE_PRESENT)&&(pt[d]&PAGE_USER))return -1;
 pt[d]=pa|PAGE_PRESENT|PAGE_WRITE|PAGE_USER;return 0;
}
/* Destroy page tables belonging to a process. Mapped user physical pages are
 * intentionally not freed here; process.c owns those pages. */
void vmm_destroy_address_space(uint64_t cr3){
 if(!cr3||cr3==(uint64_t)kernel_pml4)return;
 uint64_t *pml4=(uint64_t*)cr3;
 for(unsigned int a=0;a<PAGE_TABLE_ENTRIES;++a)if(pml4[a]&PAGE_PRESENT){
  uint64_t *pdpt=(uint64_t*)(pml4[a]&~0xFFFULL);
  for(unsigned int b=0;b<PAGE_TABLE_ENTRIES;++b)if(pdpt[b]&PAGE_PRESENT){
   if(pdpt[b]&PAGE_HUGE)continue;
   uint64_t *pd=(uint64_t*)(pdpt[b]&~0xFFFULL);
   for(unsigned int c=0;c<PAGE_TABLE_ENTRIES;++c)if(pd[c]&PAGE_PRESENT&&!(pd[c]&PAGE_HUGE))free_table((uint64_t*)(pd[c]&~0xFFFULL));
   free_table(pd);
  }
  free_table(pdpt);
 }
 free_table(pml4);
}
