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
struct coremap_page **coremap; 
static struct spinlock coremap_lock = SPINLOCK_INITIALIZER;
int num_coremap_pages; 

void vm_bootstrap(void) {

    // initialize coremap
    // get physical size of system using ram_getsize

    // this call doesnt need to be synchronised 
    paddr_t last_addr = ram_getsize(); 
    paddr_t first_addr = ram_getfirstfree(); 

    /* Manually assigning physical memory for the core map*/
    // coremap = (const struct coremap_page*) PADDR_TO_KVADDR(first_addr);
    coremap_pages = (struct coremap_page*) PADDR_TO_KVADDR(first_addr);

    spinlock_acquire(coremap_lock); 
    coremap = &coremap_pages; 

    /* Total number of pages on system */
    num_coremap_pages = (last_addr - first_addr) / PAGE_SIZE; 
    // free_addr = first_addr + num_coremap_pages * ROUNDUP(sizeof(struct coremap_entry), PAGE_SIZE); 
    free_addr = first_addr + num_coremap_pages * sizeof(struct coremap_entry); 

    for (int i = 0; i < num_coremap_pages; i++) {
        // if the first addr isn't 0, then that means that some pages are already in use by the current proc
        if (i < (first_addr - 0) / PAGE_SIZE) {
            coremap[i]->status = DIRTY; 
            coremap[i]->p_addr = i * PAGE_SIZE; 
        } else {
            if (i < (free_addr - first_addr) / PAGE_SIZE) {
                coremap[i]->status = FIXED; 
            } else {
                coremap[i]->status = FREE; 
            }
            coremap[i]->p_addr = first_addr + i * PAGE_SIZE; 
        }
        coremap[i]->v_addr = PADDR_TO_KVADDR(coremap[i]->p_addr); // check this!!!!!
    }

    spinlock_release(coremap_lock); 

}

int vm_fault(int faulttype, vaddr_t faultaddress) {
    (void) faulttype;
    (void) faultaddress;

    

    return 0;
}

vaddr_t alloc_kpages(unsigned npages) {
    (void) npages;

    // called by kmalloc
    // When a physical page is first allocated, its state is DIRTY, not CLEAN. Since this page do not 
    // have a copy in swap file (disk). Remember that in a virtual memory system, memory is just a cache of disk.

    return 0;
}

void free_kpages(vaddr_t addr) {
    (void) addr;
    //When you need to evict a page, you look up the physical address in the core map, locate the address space whose page 
    // you are evicting and modify the corresponding state information to indicate that the page will no longer be in memory.
    //  If the page is dirty, it must first be written to the backing store or swap (discussed below).
    // https://www.eecg.utoronto.ca/~yuan/teaching/ece344/asst4.html

}

void vm_tlbshootdown_all(void) {

}

void vm_tlbshootdown(const struct tlbshootdown *tlbshootdown) {
    (void) tlbshootdown;
}