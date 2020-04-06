#include "my_vm.h"

char *physical_map;//bit map for physical pages
char *virtual_map;//bit map for virtual pages
char *physical_mem;//byte array of physical memory
pde * page_directory;//page directory (array of page directory entries)
tlb * tlb_list;
bool initialized = false;//global boolean, set to true after first a_malloc call
int pgbits;
int ptbits;
int pdbits;
//pthread_mutex_t lock;

/*TLB Miss Rate Globals*/
unsigned tlbLookup = 0, tlbMiss = 0;

// Example 1 EXTRACTING TOP-BITS (Outer bits)

static unsigned int get_top_bits(unsigned int value)
{
    //Assume you would require just the higher order (outer)  bits, 
    //that is first few bits from a number (e.g., virtual address) 
    //So given an  unsigned int value, to extract just the higher order (outer)  “num_bits”
    int num_bits_to_prune = 32 - pdbits; //32 assuming we are using 32-bit address 
    return (value >> num_bits_to_prune);
}


//Example 2 EXTRACTING BITS FROM THE MIDDLE
//Now to extract some bits from the middle from a 32 bit number, 
//assuming you know the number of lower_bits (for example, offset bits in a virtual address)

static unsigned int get_mid_bits (unsigned int value)
{

   //value corresponding to middle order bits we will returning.
   unsigned int mid_bits_value = 0;   

   // First you need to remove the lower order bits (e.g. PAGE offset bits). 
   value =    value >> pgbits; 


   // Next, you need to build a mask to prune the outer bits. How do we build a mask?   

   // Step1: First, take a power of 2 for “num_middle_bits”  or simply,  a left shift of number 1.  
   // You could try this in your calculator too.
   unsigned int outer_bits_mask =   (1 << ptbits);  

   // Step 2: Now subtract 1, which would set a total of  “num_middle_bits”  to 1 
   outer_bits_mask = outer_bits_mask-1;

   // Now time to get rid of the outer bits too. Because we have already set all the bits corresponding 
   // to middle order bits to 1, simply perform an AND operation. 
   mid_bits_value =  value &  outer_bits_mask;

  return mid_bits_value;

}

//Example 3
//Function to set a bit at "index"
// bitmap is a region where were store bitmap 
static void set_bit_at_index(char *bitmap, int index, bool on)
{
    // We first find the location in the bitmap array where we want to set a bit
    // Because each character can store 8 bits, using the "index", we find which 
    // location in the character array should we set the bit to.
    char *region = ((char *) bitmap) + (index / 8);

    // Now, we cannot just write one bit, but we can only write one character. 
    // So, when we set the bit, we should not distrub other bits. 
    // So, we create a mask and OR with existing values
    char bit = 1 << (index % 8);

    // just set the bit to 1. NOTE: If we want to free a bit (*bitmap_region &= ~bit;)
    if(on){

        *region |= bit;

    }else{

        *region &= ~bit;

    }

    return;
}


//Example 3
//Function to get a bit at "index"
static int get_bit_at_index(char *bitmap, int index)
{
    //Same as example 3, get to the location in the character bitmap array
    char *region = ((char *) bitmap) + (index / 8);

    //Create a value mask that we are going to check if bit is set or not
    char bit = 1 << (index % 8);

    return (int)(*region >> (index % 8)) & 0x1;
}

/*
Function responsible for allocating and setting your physical memory 
*/
void set_physical_mem() {

    pgbits = (int)log2(PGSIZE);
    ptbits = pgbits-(int)log2(sizeof(pte));
    pdbits = 32-pgbits-ptbits;

    //Allocate physical memory using mmap or malloc; this is the total size of
    //your memory you are simulating
    physical_mem = (unsigned char*)(malloc(sizeof(char)*MEMSIZE));
    physical_map = (unsigned char*)(calloc(((MEMSIZE/PGSIZE)/8),sizeof(char)));
    virtual_map = (unsigned char*)(calloc(((MAX_MEMSIZE/PGSIZE)/8),sizeof(char)));

    tlb_list = (tlb*)(malloc(sizeof(tlb)*TLB_ENTRIES));
    unsigned c = 0;
    for(c=0;c<TLB_ENTRIES;c++){

        (tlb_list[c]).valid = false;

    }

    //memset(physical_map,0,sizeof(char)*((MEMSIZE/PGSIZE)/8));//zero out bit maps
    //memset(virtual_map,0,sizeof(char)*((MAX_MEMSIZE/PGSIZE)/8));

    unsigned int num_of_pde = (unsigned int)(pow(2,pdbits));
    page_directory = (pde*)(malloc(sizeof(pde)*num_of_pde));

    for(c=0;c<num_of_pde;c++){

        (page_directory[c]).pagetable = NULL;

    }
    
	/*if (pthread_mutex_init(&lock, NULL) != 0) {
		printf("\n mutex init has failed\n");
	}*/

	initialized = true;

    //initialize page directory and page tables
    //HINT: Also calculate the number of physical and virtual pages and allocate
    //virtual and physical bitmaps and initialize them

}


/*
 * Part 2: Add a virtual to physical page translation to the TLB.
 * Feel free to extend the function arguments or return type.
 */
int add_TLB(void *va, void *pa)
{

    /*Part 2 HINT: Add a virtual to physical page translation to the TLB */
    unsigned int map = ((unsigned int)va)>>pgbits;
    if(tlb_list[map%TLB_ENTRIES].valid){

        remove_TLB(va);

    }
    (tlb_list[map%TLB_ENTRIES]).valid = true;
    (tlb_list[map%TLB_ENTRIES]).va = va;
    (tlb_list[map%TLB_ENTRIES]).pa = pa;
    return 0;
}

int remove_TLB(void *va)
{

	/*Part 2 HINT: Remove a virtual to physical page translation from the TLB */
    unsigned int map = ((unsigned int)va)>>pgbits;
    (tlb_list[map%TLB_ENTRIES]).va=NULL;
    (tlb_list[map%TLB_ENTRIES]).pa=NULL;
    (tlb_list[map%TLB_ENTRIES]).valid = false;
	return 0;
}


/*
 * Part 2: Check TLB for a valid translation.
 * Returns the physical page address.
 * Feel free to extend this function and change the return type.
 */
void * check_TLB(void *va) {
	tlbLookup++;
	/* Part 2: TLB lookup code here */
    unsigned int map = ((unsigned int)va)>>pgbits;
    if((tlb_list[map%TLB_ENTRIES]).valid==false){
		tlbMiss++;
        return NULL;

    }
    return (tlb_list[map%TLB_ENTRIES]).pa;

}


/*
 * Part 2: Print TLB miss rate.
 * Feel free to extend the function arguments or return type.
 */
void print_TLB_missrate()
{
	double miss_rate = 0; /*TLB Misses / TLB Lookups*/
	miss_rate = ((double)tlbMiss / tlbLookup); // inc in checkTLB
	/*Part 2 Code here to calculate and print the TLB miss rate*/

	fprintf(stderr, "TLB miss rate %lf \n", miss_rate);
}



/*
The function takes a virtual address and page directories starting address and
performs translation to return the physical address
*/
void *translate(void *va) {
	/* Part 1 HINT: Get the Page directory index (1st level) Then get the
	* 2nd-level-page table index using the virtual address.  Using the page
	* directory index and page table index get the physical address.
	*
	* Part 2 HINT: Check the TLB before performing the translation. If
	* translation exists, then you can return physical address from the TLB.
	*/

	void * translation = check_TLB(va);
	if (translation != NULL)
		return translation; //In TLB so just return straight away

	translation = ((page_directory[get_top_bits((unsigned)va)]).pagetable[get_mid_bits((unsigned)va)]).paddr;

	//TLB CACHE
	add_TLB(va, translation);

	return translation;

	//If translation not successful
	//return NULL; 
}


/*
The function takes a page directory address, virtual address, physical address
as an argument, and sets a page table entry. This function will walk the page
directory to see if there is an existing mapping for a virtual address. If the
virtual address is not present, then a new entry will be added
*/
int page_map(void *va, void *pa)
{

	unsigned int pd = get_top_bits((unsigned int)va);
	unsigned int pt = get_mid_bits((unsigned int)va);
    
	unsigned int num_of_pte = (unsigned int)(pow(2, ptbits));

	if (((page_directory[pd]).pagetable) == NULL) {

		(page_directory[pd]).pagetable = (pte*)(malloc(sizeof(pte)*num_of_pte));
		unsigned c;
		for (c = 0; c < num_of_pte; c++) {

			((page_directory[pd]).pagetable)[c].paddr = NULL;

		}

	}

	((page_directory[pd]).pagetable)[pt].paddr = pa;


	return 0;

}


/*Function that gets the next available page
*/
void *get_next_avail(unsigned num_pages, unsigned char* bitmap, unsigned long long memSize) {
	unsigned chain = 0, index, avail = 0, bit;
	for (index = 0; index < ((memSize / PGSIZE) / 8); index++) {
		if (bitmap[index] == 0b11111111)
			continue;//optimization to skip 8 unnecessary checks
		for (bit = 0; bit < 8; bit++) {
			if (((bitmap[index] >> bit) & 1) == 0) {
				if (chain == 0)
					avail = index * 8 + bit;
				chain++;
			}
			else
				chain = 0;
			if (chain == num_pages) {
				//claim the bits
				unsigned x;
				index = avail / 8;
				bit = avail % 8;
				for (x = 1; x <= chain; x++) {
					bitmap[index] |= 1 << bit;
					index = (avail+x) / 8;
					bit = (avail+x) % 8;
				}

				return (void *)((avail*PGSIZE)+1);
			}
		}
	}
	return NULL;
    //Use virtual address bitmap to find the next free page
}


/* Function responsible for allocating pages
and used by the benchmark
*/
void *a_malloc(unsigned int num_bytes) {
	//input validation
	if (num_bytes < 1)
		return NULL;

    /* 
     * HINT: If the physical memory is not yet initialized, then allocate and initialize.
     */

	if (!initialized)
		set_physical_mem();

   /* 
    * HINT: If the page directory is not initialized, then initialize the
    * page directory. Next, using get_next_avail(), check if there are free pages. If
    * free pages are available, set the bitmaps and map a new page. Note, you will 
    * have to mark which physical pages are used. 
    */

	//unsigned numPages = ceil((double)(num_bytes / PGSIZE));
	unsigned numPages = (num_bytes / PGSIZE) + 1;
	//pthread_mutex_lock(&lock);
	void* va = get_next_avail(numPages, virtual_map, MAX_MEMSIZE);
	//pthread_mutex_unlock(&lock);
	if (va == NULL)
		return NULL; // no space or no consecutive space for the request
	//pthread_mutex_lock(&lock);
	void* pa = get_next_avail(numPages, physical_map, MEMSIZE);
	//pthread_mutex_unlock(&lock);
	if (pa == NULL)
		return NULL; //not enough physical space
	pa = physical_mem + (unsigned long)pa;
	unsigned x;
	void* firstPageVA = va;
	for (x = 0; x < numPages; x++) {
		page_map(va, pa);
		va = (void*)((char*)va + PGSIZE);
		pa = (void*)((char*)pa + PGSIZE);
	}
	
	return firstPageVA;

}

/* Responsible for releasing one or more memory pages using virtual address (va)
*/
void a_free(void *va, int size) {
	//input validation
	if((unsigned int)va < 1 || size < 1 || ((unsigned int)(va) + size) > pow(2, 32))
		return;

    /* Part 1: Free the page table entries starting from this virtual address
     * (va). Also mark the pages free in the bitmap. Perform free only if the 
     * memory from "va" to va+size is valid.
     *
     * Part 2: Also, remove the translation from the TLB
     */

	//memset(physical_mem[(unsigned)translate(va)], 0, size);//clear contents

	unsigned numPages = (size / PGSIZE) + 1;

	void *pa = translate(va);
	unsigned page = (unsigned)(pa) / PGSIZE;
	unsigned index = page / 8;
	unsigned bit = page % 8;
	unsigned x;
	//pthread_mutex_lock(&lock);
	for (x = 0; x < numPages; x++) {
		if (((physical_map[index] >> bit) & 1) == 1) {
			physical_map[index] &= ~(1 << bit);
		}
		page++;
		bit = page % 8;
		index = page / 8;
	}
	//pthread_mutex_unlock(&lock);

	page = (unsigned)(va) / PGSIZE;
	index = page / 8;
	bit = page % 8;

	//pthread_mutex_lock(&lock);
	for (x = 0; x < numPages; x++) {
		if (((virtual_map[index] >> bit) & 1) == 1) {
			virtual_map[index] &= ~(1 << bit);
		}
		page++;
		bit = page % 8;
		index = page / 8;
	}
	//pthread_mutex_unlock(&lock);

	remove_TLB(va);
    
}


/* The function copies data pointed by "val" to physical
* memory pages using virtual address (va)
*/
void put_value(void *va, void *val, int size) {

   /* HINT: Using the virtual address and translate(), find the physical page. Copy
    * the contents of "val" to a physical page. NOTE: The "size" value can be larger 
    * than one page. Therefore, you may have to find multiple pages using translate()
    * function.
    */

   int pages = (int)(ceil((double)size/PGSIZE));
   int c=0;
   for(c=0;c<pages;c++){

       unsigned int addr = (unsigned int)va + (c*PGSIZE);
       void * pa = translate((void*)addr);
       addr = (unsigned int)val + (c*PGSIZE);
       if(c==(pages-1)){

            memcpy(pa,(void*)addr,size%PGSIZE);

       }else{

            memcpy(pa,(void*)addr,PGSIZE);

       }

   }


}


/*Given a virtual address, this function copies the contents of the page to val*/
void get_value(void *va, void *val, int size) {

   /* HINT: put the values pointed to by "va" inside the physical memory at given
   * "val" address. Assume you can access "val" directly by derefencing them.
   */

   int pages = (int)(ceil((double)size/PGSIZE));
   int c=0;
   for(c=0;c<pages;c++){

       unsigned int addr = (unsigned int)va + (c*PGSIZE);
       void * pa = translate((void*)addr);
       addr = (unsigned int)val + (c*PGSIZE);
       if(c==(pages-1)){

            memcpy((void*)addr,pa,size%PGSIZE);

       }else{

            memcpy((void*)addr,pa,PGSIZE);

       }

   }



}



/*
This function receives two matrices mat1 and mat2 as an argument with size
argument representing the number of rows and columns. After performing matrix
multiplication, copy the result to answer.
*/
void mat_mult(void *mat1, void *mat2, int size, void *answer) {

   /* Hint: You will index as [i * size + j] where  "i, j" are the indices of the
    * matrix accessed. Similar to the code in test.c, you will use get_value() to
    * load each element and perform multiplication. Take a look at test.c! In addition to 
    * getting the values from two matrices, you will perform multiplication and 
    * store the result to the "answer array"
    */
   int i = 0;
   int j = 0;
   int k = 0;

   for(i=0;i<size;i++){
       for(j=0;j<size;j++){
           int add = 0;
           unsigned address_ans = (unsigned int)answer + ((i * size * sizeof(int))) + (j * sizeof(int));
           for(k=0;k<size;k++){

               unsigned int address_mat1 = (unsigned int)mat1 + ((i * size * sizeof(int))) + (k * sizeof(int));
               unsigned int address_mat2 = (unsigned int)mat2 + ((k * size * sizeof(int))) + (j * sizeof(int));
               int val_mat1;
               int val_mat2;
               get_value((void *)address_mat1,(void*)&val_mat1, sizeof(int));
               get_value((void *)address_mat2, (void*)&val_mat2, sizeof(int));
               add += (val_mat1)*(val_mat2);
               put_value((void *)address_ans, (void*)&add, sizeof(int));


           }
       }
   }
      
}



