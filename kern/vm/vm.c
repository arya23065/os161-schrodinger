#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <spl.h>
#include <spinlock.h>
#include <proc.h>
#include <current.h>
#include <mips/tlb.h>
#include <addrspace.h>
#include <vm.h>


// start address from which onward, one can allocate physical pages
paddr_t free_addr;

// Coremap - an array of coremap pages / physical pages
struct coremap_page *coremap;
static struct spinlock coremap_lock = SPINLOCK_INITIALIZER;         // To initialize spinlock during boot
unsigned long num_coremap_pages;                                    // Number of physical pages
bool bootstrap_done;                                                // Boolean to signify if VM is initialized or not

void vm_bootstrap(void) {

    paddr_t last_addr = ram_getsize();          // Gives us last physical address
    paddr_t first_addr = ram_getfirstfree();    // Gives us first free physical address to know how much memory we have that we need to manage.

    // Manually assigning physical memory for the core map
    coremap = (struct coremap_page*) PADDR_TO_KVADDR(first_addr);

    spinlock_acquire(&coremap_lock); 

    // Total number of physical pages on system
    num_coremap_pages = last_addr / PAGE_SIZE; 
    // The first free address after memory is allocated to store coremap struct
    free_addr = first_addr + num_coremap_pages * sizeof(struct coremap_page); 

    // Aligning free_addr so that it is the base address of a page
    free_addr = free_addr + PAGE_SIZE - (free_addr % PAGE_SIZE); 

    unsigned long coremap_pages_initialised = 0; 

    // Initializing all coremap_page entries
    for (unsigned long i = 0; i < num_coremap_pages; i++) {
        // Set page status to dirty if page is before first_addr because these pages are already in use
        if (i < (first_addr - 0) / PAGE_SIZE) {
            coremap[i].status = DIRTY; 
        }
        // Set all pages used for storing coremap struct to FIXED so that they can't be freed
        else {
            if (coremap_pages_initialised != ((free_addr - first_addr) / PAGE_SIZE)) {
                coremap_pages_initialised++;
                coremap[i].status = FIXED;
            }
            // All other pages are set to FREE
            else {
                coremap[i].status = FREE;
            }
        }

        // Initialize the physical and virtual base address of all pages
        coremap[i].p_addr = i * PAGE_SIZE; 
        coremap[i].v_addr = PADDR_TO_KVADDR(coremap[i].p_addr); // check this!!!!!
        coremap[i].block_len = 0; 
    }

    coremap[0].status = DIRTY;

    spinlock_release(&coremap_lock);

    // Set bootstrap_done to true so that other functions know that they can use vm functions instead of ram functions
    bootstrap_done = true;

    return;

}

int vm_fault(int faulttype, vaddr_t faultaddress) {
    (void) faulttype;
    (void) faultaddress;

    return 0;
}


static
paddr_t
getppages(unsigned long npages) {

    paddr_t addr;
    unsigned long ppage_got = 0;
    bool first_ppage_got = false; 
    bool space_found = false; 
    unsigned long max_block_found = 0; 
    unsigned long curr_block_len = 0; 
    unsigned long max_block_page_i = 0; 

    if (bootstrap_done) {
        spinlock_acquire(&coremap_lock); 

        for (unsigned long i = 0; i < num_coremap_pages; i++) {
            if (ppage_got == npages) {      // Exit loop if we got the pages we need
                break; 
            }
            // Use replacement policy if we don't have enough free pages
            // Find the largest block of free contiguous memory that is less than npages and evict pages after it to make enough space so that we have npages free pages
            if (i == num_coremap_pages - 1 && (npages > 1 || (npages == 1 && coremap[i].status != FREE)) ) {

                addr = coremap[max_block_page_i].p_addr; 
                coremap[max_block_page_i].block_len = npages;

                // Evict the number of pages we need
                for (unsigned long j = 0; j < npages; j++) {
                    if (coremap[max_block_page_i + j].status != FREE) {
                        KASSERT(coremap[max_block_page_i + j].status != FIXED); 
                    }
                    coremap[max_block_page_i + j].status = DIRTY; 
                }

                break; 


            }
            // Find the number of contiguous pages of physical memory we have if the current page is free
            else if (coremap[i].status == FREE) {
                curr_block_len = 0; 

                // Check if we found enough space
                for (unsigned long j = 0; j < npages; j++) {

                    // If we find the largest block of free memory less than npages, set parameters for replacement policy and use it
                    if (coremap[i + j].status != FREE) {
                        space_found = false; 
                        curr_block_len++; 

                        if (curr_block_len > max_block_found) {
                            max_block_found = curr_block_len; 
                            max_block_page_i = i;                   // Set parameters for replacement policy (see above) if enough space isn't found
                        }

                        break;
                    }
                    // If we have enough space, set space_found to true so that we don't try to use replacement policy
                    else if (coremap[i + j].status == FREE && j == npages - 1) {
                        space_found = true; 
                    }
                }

                // If we found enough space, allocate it by setting status of pages to DIRTY
                if (space_found) {
                    if (!first_ppage_got) {
                        addr = coremap[i].p_addr; 
                        first_ppage_got = true; 
                        coremap[i].block_len = npages; // only the first page of the block hold the block len
                    }
                    coremap[i].status = DIRTY; 
                    ppage_got++; 
                }
            }
        }
        
        spinlock_release(&coremap_lock); 
        
    } else {
        // if the core map hasn't been initialized, we must directly take pages from the ram, without any action related to the coremap
        addr = ram_stealmem(npages);
    }

    return addr;
}

vaddr_t alloc_kpages(unsigned npages) {
    // called by kmalloc

	paddr_t pa;
	pa = getppages(npages);     // Get the base physical address of memory allocated by getppages
	if (pa == 0) {
		return 0;               // Return 0 (NULL) if base physical address is 0
	}
	return PADDR_TO_KVADDR(pa); // Return virtual address translation of physical address if getppages is successful
}

void free_kpages(vaddr_t addr) {

    spinlock_acquire(&coremap_lock); 
    unsigned long npages = 0;
    unsigned long pages_freed = 0; 
    bool page_found = false; 

    // Free pages
    for (unsigned long i = 0; i < num_coremap_pages; i++) {
        if (!page_found) {
            if (coremap[i].v_addr == addr) {
                KASSERT(coremap[i].status != FIXED); // fixed pages cannot be freed
                coremap[i].status = FREE;           // If we found first page of block that we want to free, set it to FREE
                npages = coremap[i].block_len;      // Find the number of pages we need to free
                pages_freed++;
                page_found = true; 
                // break; 
            }
        } else {
            if (npages == 0) {      // Don't free anything if only 0 or 1 page(s) need to be freed and exit loop
                break; 
            }
            if (pages_freed == npages) {    // Exit loop if number of pages freed is npages
                break;
            } else {                        // Set page status to free if it belongs to the block of memory we are trying to free and update the number of pages we have freed
                coremap[i].status = FREE; 
                pages_freed++; 
            }

        }

    }

    spinlock_release(&coremap_lock); 
}

void vm_tlbshootdown_all(void) {

}

void vm_tlbshootdown(const struct tlbshootdown *tlbshootdown) {
    (void) tlbshootdown;
}