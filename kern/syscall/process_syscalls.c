#include <types.h>
#include <lib.h>
#include <kern/fcntl.h>
#include <copyinout.h>
#include <syscall.h>
#include <proc.h>
#include <current.h>
#include <unistd.h>
#include <addrspace.h>
#include <proc.h>
#include <pid_table.h>


int 
sys_getpid(pid_t *retval) {
    *retval = curproc->p_pid;  
    return 0;   // sys_getpid does not fail
}

int
sys_fork(struct trapframe *tf, pid_t *retval) {
    // create new proc 
    // copy addrs space - struct addrspace - as copy dumbvm.c 
    int err = 0;

    char* name = strcat(curproc->p_name, "_child"); 
    struct proc *newproc; 

    newproc = proc_create(name);

    // Copy open file table so that each fd points to same open file
    int i = 0;
    while (i < OPEN_MAX) {
        newproc->open_filetable->open_files[i] = curproc->p_one_filetable->open_files[i];
        i++;
    }

    // copy architectural state
    spinlock_acquire(newproc->p_lock);
    struct addrspace *new_addrspace = kmalloc(sizeof(struct addrspace));
    err = as_copy(curproc->p_addrspace, &*new_addrspace);
    spinlock_release(newproc->p_lock);

    if (err) {
        proc_destroy(newproc);
        *retval = -1;
        return err;
    }
    
    newproc->p_addrspace = new_addrspace;

    *retval = pid_table_add(newproc, &err); // make sure that you remove this process from the pid table if there's an error in any of the functions used after

    if (err) {
        proc_destroy(newproc);
        *retval = -1;
        return err;
    }

    struct trapframe new_tf = *tf;
    new_tf.tf_v0 = 0;
    new_tf.tf_a3 = 0;
    new_tf.tf_epc += 4;     // Setting program counter so that same instruction doesn't run again

    // Copy kernel thread; return to user mode
    thread_fork(newproc->p_name, newproc, child_fork(&new_tf, NULL), (void*) &new_tf, NULL); 
    // return 
    return 0;
    // copy kernel thread 
    // both parent and child need to return to user mode 
}

void child_fork(void *tf, unsigned long arg) {
    struct trapframe *new_tf;
    new_tf = tf;

    // struct addrspace *new_as = (struct addrspace *) as;
    // as_activate(); 
    mips_usermode(new_tf); 
}

int sys_execv(const_userptr_t program, const_userptr_t args, int *retval) {

    /*
    * 1. copy arguments from the old address space - use copyin/out, and 'write your own function' - 
    * - Where are the arguments located when exec is invoked? in the stack of the process that invoked the syscall - we have user pointers, 
    * use them as we did for write and read 
    * - In whose address space are they? In the addr space for that process 
    * - How do you know their addresses? String array, program name, pointer to an array of strings
    * - How do we get them into the kernel? Copyin
    * - How do we know their total size? Copyin returns the size 
    * - Where do the arguments need to end up before we return to the user mode? Whose address space? Stack or heap? 
    * Copy out the arguments. In addition to kernel's heap(copy in), copy it out into the user stack 
    * 
    * Must copy in arguments from the old addrspace, then copy them out into the new addrspace
    * 
    * 2. get a new address space 
    * 3. switch to the new address space   
    * 4. load a new executable 
    * 5. define a new stack region
    * - check as_define_stack
    * 
    * 6. copy the arguments into the new address space, properly arranging them
    * - Place new args on user space stakc (when you create a new process you have a pointer to this), decrement 
    * the SP depending on the size of arguments (copy strings AND pointers)
    * 
    * 7. clean up the old address space 
    * 8. return to user mode - consider using enter new process since mipsusermode only uses the trapframe 
    * - use enter_new_process
    * - Passed in registers - set this up in the trapframe before going to user mode 
    * 
    * 
    */
    
}
