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

    *retval = pid_table_add(newproc, &err);

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

    // return 
    return 0;
    // copy file table - shallow copy of open files, deep copy of file table
    // copy architectural state 
    // copy kernel thread 
    // both parent and child need to return to user mode 

}

void child_fork(void *as, unsigned long tf) {
    struct trapframe *new_tf;
    new_tf = tf;

    struct addrspace *new_as = (struct addrspace *) as;


}