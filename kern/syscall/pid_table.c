#include <types.h>
#include <lib.h>
#include <limits.h>
#include <pid_table.h>
#include <proc.h>
#include <synch.h>
#include <kern/errno.h>
#include <current.h>

struct pid_table *kpid_table;

void
pid_table_init(void) {

    kpid_table = kmalloc(sizeof(struct pid_table));
    if (kpid_table == NULL) {
        panic("Failed to initialize PID table");
    }

    for (int i = 0; i <= PID_MAX; i++) {
        kpid_table->pid_array[i] = NULL;
        kpid_table->exit_array[i] = NULL;
    }

    kpid_table->pid_table_lock = lock_create("PID Table lock");
    if (kpid_table->pid_table_lock == NULL) 
        panic("Failed to initialize PID table");

    lock_acquire(kpid_table->pid_table_lock);

    kpid_table->pid_array[1] = kproc;
    kproc->p_pid = 1;

    kpid_table->exit_array[1] = kmalloc(sizeof(struct p_exit_info));
    KASSERT(kpid_table->exit_array[1] != NULL);
    kpid_table->exit_array[1]->has_exited = false;
    kpid_table->exit_array[1]->parent_pid = 0;
    kpid_table->exit_array[1]->exit_cv = cv_create("KPROC CV");
    kpid_table->exit_array[1]->has_exited = -1;
    KASSERT(kpid_table->exit_array[1]->exit_cv != NULL);

    lock_release(kpid_table->pid_table_lock);
}

// EDIT WITH STUFF FROM EXIT_ARRAY
void 
pid_table_destroy(void) {

    KASSERT(kpid_table != NULL); 

    lock_acquire(kpid_table->pid_table_lock);
    for (int i = 0; i < PID_MAX; i++) {
        if (kpid_table->pid_array[i] != NULL)
            kfree(kpid_table->pid_array[i]);

        if (kpid_table->exit_array[i] != NULL)
            kfree(kpid_table->exit_array[i]);
    }
    lock_release(kpid_table->pid_table_lock);

    lock_destroy(kpid_table->pid_table_lock);
    kfree(kpid_table);
}

pid_t 
pid_table_add(struct proc *proc, int *err) {

    pid_t ret_pid = -1;
    *err = 0;

    lock_acquire(kpid_table->pid_table_lock); 

    int i = PID_MIN; 

    /* The pid 0 is reserved, so we must start assigning PIDs from index 1 */
    while (i <= PID_MAX) {
        if (kpid_table->pid_array[i] == NULL && kpid_table->exit_array[i] == NULL) {
            kpid_table->pid_array[i] = proc;
            
            kpid_table->exit_array[i] = kmalloc(sizeof(struct p_exit_info));
            kpid_table->exit_array[i]->parent_pid = curproc->p_pid;
            kpid_table->exit_array[i]->has_exited = false;
            kpid_table->exit_array[i]->exitcode = -1;
            kpid_table->exit_array[i]->exit_cv = cv_create("Exit CV");
            
            if (kpid_table->exit_array[i]->exit_cv == NULL) {
                kfree(kpid_table->exit_array[i]);
                *err = ENOMEM;
                break;
            }
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

// EDIT WHILE WRITING EXIT SYSCALL
int
pid_table_delete(pid_t pid) {
    KASSERT(kpid_table != NULL);

    lock_acquire(kpid_table->pid_table_lock);

    if (kpid_table->pid_array[pid] != NULL) {
        proc_destroy(kpid_table->pid_array[pid]);
    }

    lock_release(kpid_table->pid_table_lock);
    return 0;
}



