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


pid_t 
sys_getpid(int *retval) {
    *retval = curproc->p_pid;  
    return 0;   // sys_getpid does not fail
}

pid_t
sys_fork(int *retval) {
    // create new proc 
    // copy addrs space - struct addrspace - as copy dumbvm.c 
    char* name = concat(curproc->p_name, "_child"); 
    struct proc *newproc; 

    newproc = proc_create(name); 
    struct addrspace *new_addrspace;
    newproc->p_name = name; 

    if (as_copy(curproc->p_addrspace, &new))

    newproc->p_addrspace = new_addrspace; 
    // copy file table - shallow copy of open files, deep copy of file 
    // copy architectural state 
    // copy kernel thread 
    // both parent and child need to return to user mode 

}