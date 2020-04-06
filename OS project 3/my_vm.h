#ifndef MY_VM_H_INCLUDED
#define MY_VM_H_INCLUDED
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
//#include <pthread.h>

//Assume the address space is 32 bits, so the max memory size is 4GB
//Page size is 4KB

//Add any important includes here which you may need

#define PGSIZE 4096

// Maximum size of virtual memory
#define MAX_MEMSIZE 4ULL*1024*1024*1024

// Size of "physcial memory"
#define MEMSIZE 1024*1024*1024

//ignore these two
// // Represents a page table entry
// typedef unsigned long pte_t;

// // Represents a page directory entry
// typedef unsigned long pde_t;

typedef struct page_table_entry{

    void * paddr;//physical address

}pte;


typedef struct page_directory_entry{

    pte*pagetable;//pointer to page table (array of page table entries)

}pde;


#define TLB_ENTRIES 512

typedef struct _tlb {
    /*Assume your TLB is a direct mapped TLB with number of entries as TLB_ENTRIES
    * Think about the size of each TLB entry that performs virtual to physical
    * address translation.
    */
   void * va;
   void * pa;
   bool valid;
	
}tlb;



void set_physical_mem();
void * translate(void *va);
int page_map(void *va, void* pa);
void * check_TLB(void *va);
int add_TLB(void *va, void *pa);
int remove_TLB(void *va);
void *a_malloc(unsigned int num_bytes);
void *get_next_avail(unsigned num_pages, unsigned char* bitmap, unsigned long long memSize);
void a_free(void *va, int size);
void put_value(void *va, void *val, int size);
void get_value(void *va, void *val, int size);
void mat_mult(void *mat1, void *mat2, int size, void *answer);
void print_TLB_missrate();

#endif
