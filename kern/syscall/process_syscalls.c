#include <types.h>
#include <lib.h>
#include <kern/fcntl.h>
#include <copyinout.h>
#include <syscall.h>
#include <proc.h>
#include <current.h>
#include <unistd.h>



pid_t 
sys_getpid(int *retval) {
    *retval = curproc->p_pid;  
    return 0;   // sys_getpid does not fail
}