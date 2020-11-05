#include <types.h>
#include <lib.h>
#include <limit.h>
#include <pid_table.h>
#include <proc.h>
#include <synch.h>
#include <kern/errno.h>


struct pid_table*
pid_table_init() {

    struct pid_table *new_pid_table; 

    new_pid_table = kmalloc(sizeof(struct pid_table));
    if (new_pid_table == NULL) {
            return NULL;
    }

    new_pid_table->pid_table_lock = lock_create("PID Table lock"); 

    return pid_table; 
}

void 
pid_table_destroy(struct pid_table *pid_table) {

    KASSERT(pid_table != NULL); 

    for (int i = 0; i < PID_MAX; i++) {
        if (pid_table->pid_array[i] != NULL)
            kfree(pid_table->pid_array[i]);
    }

    lock_destroy(pid_table->pid_table_lock);

    kfree(pid_table);
    return 0;
    
}

pid_t 
pid_table_add(struct proc *proc, int *err) {

    pid_t ret_pid = -1; 

    lock_acquire(kpid_table->pid_table_lock); 

    int i = 1; 

    /* The pid 0 is reservered, so we must start assigning PIDs from index 1 */
    while (i <= PID_MAX) {
        if (kpid_table->pid_array[i] == NULL) {
            kpid_table->pid_array[i] = proc; 
            ret_pid = i; 
            break; 
        }
        i++; 
    }

    lock_release(kpid_table->pid_table_lock); 

    if (i == PID_MAX + 1) {
        *err = ENPROC; 
    }

    return ret_pid; 
}



