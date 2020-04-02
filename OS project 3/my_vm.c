#include "my_vm.h"

char *physical_map;//bit map for physical pages
char *virtual_map;//bit map for virtual pages
char *physical_mem;//byte array of physical memory
pde * page_directory;//page directory (array of page directory entries)
bool initialized = false;//global boolean, set to true after first a_malloc call


// Example 1 EXTRACTING TOP-BITS (Outer bits)

static unsigned int get_top_bits(unsigned int value)
{
    //Assume you would require just the higher order (outer)  bits, 
    //that is first few bits from a number (e.g., virtual address) 
    //So given an  unsigned int value, to extract just the higher order (outer)  “num_bits”
    int num_bits_to_prune = 32 - PDBITS; //32 assuming we are using 32-bit address 
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
   value =    value >> PGBITS; 


   // Next, you need to build a mask to prune the outer bits. How do we build a mask?   

   // Step1: First, take a power of 2 for “num_middle_bits”  or simply,  a left shift of number 1.  
   // You could try this in your calculator too.
   unsigned int outer_bits_mask =   (1 << PTBITS);  

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

void init(){

    unsigned int num_of_pde = (unsigned int)(pow(2,PDBITS));
    unsigned int num_of_pte = (unsigned int)(pow(2,PTBITS));

    page_directory = (pde*)(malloc(sizeof(pde)*num_of_pde));
    int c;
    for(c=0;c<num_of_pde;c++){

        (page_directory[c]).pagetable = (pte*)(malloc(sizeof(pte)*num_of_pte));
        int i = 0;
        for(i=0;i<num_of_pte;i++){

            (((page_directory[c]).pagetable)[i]).paddr = 0;

        }

    }

}
/*
Function responsible for allocating and setting your physical memory 
*/
void set_physical_mem() {

    //Allocate physical memory using mmap or malloc; this is the total size of
    //your memory you are simulating
    physical_mem = (char*)(malloc(sizeof(char)*MEMSIZE));
    physical_map = (char*)(malloc(sizeof(char)*((MEMSIZE/PGSIZE)/8)));
    virtual_map = (char*)(malloc(sizeof(char)*((MAX_MEMSIZE/PGSIZE)/8)));

    memset(physical_map,0,sizeof(char)*((MEMSIZE/PGSIZE)/8));//zero out bit maps
    memset(virtual_map,0,sizeof(char)*((MAX_MEMSIZE/PGSIZE)/8));
    
    init();//initialize page directory and page tables
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

    return -1;
}


/*
 * Part 2: Check TLB for a valid translation.
 * Returns the physical page address.
 * Feel free to extend this function and change the return type.
 */
pte_t * check_TLB(void *va) {

    /* Part 2: TLB lookup code here */

	return NULL;

}


/*
 * Part 2: Print TLB miss rate.
 * Feel free to extend the function arguments or return type.
 */
void print_TLB_missrate()
{
    double miss_rate = 0;	

    /*Part 2 Code here to calculate and print the TLB miss rate*/




    fprintf(stderr, "TLB miss rate %lf \n", miss_rate);
}



/*
The function takes a virtual address and page directories starting address and
performs translation to return the physical address
*/
pte_t *translate(pde_t *pgdir, void *va) {
    /* Part 1 HINT: Get the Page directory index (1st level) Then get the
    * 2nd-level-page table index using the virtual address.  Using the page
    * directory index and page table index get the physical address.
    *
    * Part 2 HINT: Check the TLB before performing the translation. If
    * translation exists, then you can return physical address from the TLB.
    */

	pte_t* translation = check_TLB(va);
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
int page_map(pde_t *pgdir, void *va, void *pa)
{

    unsigned int pd = get_top_bits((unsigned int)va);
    unsigned int pt = get_mid_bits((unsigned int)va);

    unsigned int pos = (pd*((unsigned int)(pow(2,PTBITS))));
    pos+=pt;

    if(pos>=((unsigned int)(pow(2,PTBITS)+pow(2,PDBITS)))){

        return -1;

    }

    if(get_bit_at_index(virtual_map,pos)){

        return -1;

    }

    set_bit_at_index(virtual_map,pos,true);
    ((page_directory[pd]).pagetable)[pt].paddr = pa;

    return 0;

}


/*Function that gets the next available page
*/
void *get_next_avail(int num_pages) {
 
    //Use virtual address bitmap to find the next free page
}


/* Function responsible for allocating pages
and used by the benchmark
*/
void *a_malloc(unsigned int num_bytes) {

    /* 
     * HINT: If the physical memory is not yet initialized, then allocate and initialize.
     */

   /* 
    * HINT: If the page directory is not initialized, then initialize the
    * page directory. Next, using get_next_avail(), check if there are free pages. If
    * free pages are available, set the bitmaps and map a new page. Note, you will 
    * have to mark which physical pages are used. 
    */

    return NULL;
}

/* Responsible for releasing one or more memory pages using virtual address (va)
*/
void a_free(void *va, int size) {

    /* Part 1: Free the page table entries starting from this virtual address
     * (va). Also mark the pages free in the bitmap. Perform free only if the 
     * memory from "va" to va+size is valid.
     *
     * Part 2: Also, remove the translation from the TLB
     */
     
    
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




}


/*Given a virtual address, this function copies the contents of the page to val*/
void get_value(void *va, void *val, int size) {

    /* HINT: put the values pointed to by "va" inside the physical memory at given
    * "val" address. Assume you can access "val" directly by derefencing them.
    */




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

       
}



