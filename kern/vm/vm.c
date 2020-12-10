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


/* start address from which onward, one can allocate physical pages */
paddr_t free_addr;

/* Coremap - an array of coremap pages / physical pages */
struct coremap_page *coremap; 
static struct spinlock coremap_lock = SPINLOCK_INITIALIZER;
unsigned long num_coremap_pages; 
bool bootstrap_done; 

void vm_bootstrap(void) {

    // initialize coremap
    // get physical size of system using ram_getsize

    // this call doesnt need to be synchronised 
    paddr_t last_addr = ram_getsize(); 
    paddr_t first_addr = ram_getfirstfree(); 

    /* Manually assigning physical memory for the core map*/
    // coremap = (const struct coremap_page*) PADDR_TO_KVADDR(first_addr);
    // struct coremap_page* coremap_first_page = (struct coremap_page*) PADDR_TO_KVADDR(first_addr);
    coremap = (struct coremap_page*) PADDR_TO_KVADDR(first_addr);

    spinlock_acquire(&coremap_lock); 
    // coremap = &coremap_first_page;

    /* Total number of pages on system */
    num_coremap_pages = (last_addr - 0) / PAGE_SIZE; 
    // free_addr = first_addr + num_coremap_pages * ROUNDUP(sizeof(struct coremap_entry), PAGE_SIZE); 
    free_addr = first_addr + num_coremap_pages * sizeof(struct coremap_page); 

    // To align the free address to the page size
    free_addr = free_addr + PAGE_SIZE - (free_addr % PAGE_SIZE); 

    // bool coremap_pages_init = false; 
    unsigned long coremap_pages_initialised = 0; 

    for (unsigned long i = 0; i < num_coremap_pages; i++) {
        // if the first addr isn't 0, then that means that some pages are already in use by the current proc
        // if (i != 0) {
        //     coremap[i] = (struct coremap_page*) PADDR_TO_KVADDR(first_addr + i * PAGE_SIZE); 
        // }

        if (i < (first_addr - 0) / PAGE_SIZE) {
            coremap[i].status = DIRTY; 
            // coremap[i]->p_addr = i * PAGE_SIZE; 
        } else {
            if (coremap_pages_initialised != ((free_addr - first_addr) / PAGE_SIZE)) {
                coremap_pages_initialised++; 
                coremap[i].status = FIXED; 
                // if (coremap_pages_initialised == (free_addr - first_addr) / PAGE_SIZE) {
                //     // coremap_pages_init = true; 
                // }
            } else {
                coremap[i].status = FREE; 
            }
            // coremap[i]->p_addr = i * PAGE_SIZE; 
        }
        coremap[i].p_addr = i * PAGE_SIZE; 
        coremap[i].v_addr = PADDR_TO_KVADDR(coremap[i].p_addr); // check this!!!!!
        coremap[i].block_len = 0; 
    }

    coremap[0].status = DIRTY; 

    spinlock_release(&coremap_lock); 

    bootstrap_done = true; 

    return; 

    // At the end of vm_bootstrap, we may want to set some flags to indicate that vm has already bootstrapped, 
    // since functions like alloc_kpages may call different routines to get physical page before and after vm_bootstrap.

}

int vm_fault(int faulttype, vaddr_t faultaddress) {
    (void) faulttype;
    (void) faultaddress;

    

    return 0;
}


static
paddr_t
getppages(unsigned long npages) {

    // spinlock_acquire(&stealmem_lock);
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
            if (ppage_got == npages) {
                break; 
            }
            if (i == num_coremap_pages - 1 && (npages > 1 || (npages == 1 && coremap[i].status != FREE)) ) {
                //replace something!!!!!
                // need a replacement policy!!!!!!!!!!!!!!! - must never be a fixed page!!!!!
                // when evicting a page, based on the replaement policy, flush its content, mark it's status as clean

                // max_block_page_i = ; 

                addr = coremap[max_block_page_i].p_addr; 
                coremap[max_block_page_i].block_len = npages;

                for (unsigned long j = 0; j < npages; j++) {
                    if (coremap[max_block_page_i + j].status != FREE) {
                        KASSERT(coremap[max_block_page_i + j].status != FIXED); 
                    }
                    coremap[max_block_page_i + j].status = DIRTY; 
                }

                break; 


            } else if (coremap[i].status == FREE) {
                // checking that there is a contiguous block of npages that are free
                curr_block_len = 0; 

                for (unsigned long j = 0; j < npages; j++) {
                    if (coremap[i + j].status != FREE) {
                        space_found = false; 
                        curr_block_len++; 

                        if (curr_block_len > max_block_found) {
                            max_block_found = curr_block_len; 
                            max_block_page_i = i; 
                        }

                        break;
                    } else if (coremap[i + j].status == FREE && j == npages - 1) {
                        space_found = true; 
                    }
                }

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
        // if the core map hasn't been initilised, we must directly take pages from the ram, without any action related to the coremap
        // later, the first free page will be > 0, indicating that pages are already in use, and we set them to dirty during coremap initialisation anyway
        addr = ram_stealmem(npages);
    }

    // spinlock_release(&stealmem_lock);
    return addr;
}

vaddr_t alloc_kpages(unsigned npages) {
    // called by kmalloc
    // When a physical page is first allocated, its state is DIRTY, not CLEAN. Since this page do not 
    // have a copy in swap file (disk). Remember that in a virtual memory system, memory is just a cache of disk.

	paddr_t pa;
	pa = getppages(npages);
	if (pa == 0) {
		return 0;
	}
	return PADDR_TO_KVADDR(pa);
}

void free_kpages(vaddr_t addr) {
    //When you need to evict a page, you look up the physical address in the core map, locate the address space whose page 
    // you are evicting and modify the corresponding state information to indicate that the page will no longer be in memory.
    //  If the page is dirty, it must first be written to the backing store or swap (discussed below).
    // https://www.eecg.utoronto.ca/~yuan/teaching/ece344/asst4.html
    (void) addr;

    spinlock_acquire(&coremap_lock); 
    unsigned long npages = 0;
    unsigned long pages_freed = 0; 
    bool page_found = false; 

    for (unsigned long i = 0; i < num_coremap_pages; i++) {
        if (!page_found) {
            if (coremap[i].v_addr == addr) {
                KASSERT(coremap[i].status != FIXED); // fixed pages cannot be freed
                coremap[i].status = FREE; 
                npages = coremap[i].block_len; 
                pages_freed++; 
                page_found = true; 
                // break; 
            }
        } else {
            if (pages_freed == npages) {
                break;
            } else {
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