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


void vm_bootstrap(void) {

}

int vm_fault(int faulttype, vaddr_t faultaddress) {
    (void) faulttype;
    (void) faultaddress;

    return 0;
}

vaddr_t alloc_kpages(unsigned npages) {
    (void) npages;

    return 0;
}

void free_kpages(vaddr_t addr) {
    (void) addr;
}

void vm_tlbshootdown_all(void) {

}

void vm_tlbshootdown(const struct tlbshootdown *tlbshootdown) {
    (void) tlbshootdown;
}