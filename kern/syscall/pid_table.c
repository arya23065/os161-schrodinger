#include <types.h>
#include <lib.h>
#include <limits.h>
#include <pid_table.h>
#include <proc.h>
#include <synch.h>
#include <kern/errno.h>
#include <current.h>

// Process table
struct pid_table *kpid_table;

/*
Initialize the process table. Put kernel process in the table with PID 1.
PID 0 should be reserved and not assigned to any process
*/
void
pid_table_init(void) {

    // Allocate space for the pid table
    kpid_table = kmalloc(sizeof(struct pid_table));
    if (kpid_table == NULL) {
        panic("Failed to initialize PID table");
    }

    // Make sure that the pid table is empty
    for (int i = 0; i <= PID_MAX; i++) {
        kpid_table->pid_array[i] = NULL;
        kpid_table->exit_array[i] = NULL;
    }

    // Initialize the lock for the pid table for synchronizationwiat
    kpid_table->pid_table_lock = lock_create("PID Table lock");
    if (kpid_table->pid_table_lock == NULL) 
        panic("Failed to initialize PID table");

    lock_acquire(kpid_table->pid_table_lock);

    // Set kernel process to have pid 1
    kpid_table->pid_array[1] = kproc;
    kproc->p_pid = 1;

    // Initialize exit information struct for the kernel process
    kpid_table->exit_array[1] = kmalloc(sizeof(struct p_exit_info));
    KASSERT(kpid_table->exit_array[1] != NULL);
    kpid_table->exit_array[1]->has_exited = false;
    kpid_table->exit_array[1]->parent_pid = 0;
    kpid_table->exit_array[1]->exit_cv = cv_create("KPROC CV");
    kpid_table->exit_array[1]->exitcode = 1;
    KASSERT(kpid_table->exit_array[1]->exit_cv != NULL);

    lock_release(kpid_table->pid_table_lock);
}

/*
Destroy process table. This is ONLY called in shutdown(). We only destroy the process table
when the system is shutting down.
*/
void 
pid_table_destroy(void) {

    KASSERT(kpid_table != NULL); 

    lock_acquire(kpid_table->pid_table_lock);
    // Free the pid table so that all processes are destroyed, and any remaining stored exit information is also destroyed
    for (int i = 0; i < PID_MAX; i++) {
        if (kpid_table->pid_array[i] != NULL)
            kfree(kpid_table->pid_array[i]);

        if (kpid_table->exit_array[i] != NULL)
            kfree(kpid_table->exit_array[i]);
    }
    lock_release(kpid_table->pid_table_lock);

    // Destroy the lock for the pid table and free the pid table
    lock_destroy(kpid_table->pid_table_lock);
    kfree(kpid_table);
}

/*
Add a process to the pid table. pid_table_lock must be acquired before this function is called
*/
pid_t 
pid_table_add(struct proc *proc, int *err) {

    pid_t ret_pid = -1;
    *err = 0;

    // Make sure that the lock is held
    KASSERT(lock_do_i_hold(kpid_table->pid_table_lock));

    int i = PID_MIN; 

    /* The pid 0 is reserved and pid 1 is for the kernel process, so we must start assigning PIDs from index 2 */
    while (i <= PID_MAX) {
        // If we find a pid that is free AND isn't storing any process's exit information, assign it to the process we are adding
        if (kpid_table->pid_array[i] == NULL && kpid_table->exit_array[i] == NULL) {
            kpid_table->pid_array[i] = proc;            // proc is stored in the pid table

            // Initialize the exit information struct for the process
            kpid_table->exit_array[i] = kmalloc(sizeof(struct p_exit_info));
            kpid_table->exit_array[i]->parent_pid = curproc->p_pid;
            kpid_table->exit_array[i]->has_exited = false;
            kpid_table->exit_array[i]->exitcode = -1;
            kpid_table->exit_array[i]->exit_cv = cv_create("Exit CV");

            // If we fail, return with an error
            if (kpid_table->exit_array[i]->exit_cv == NULL) {
                kfree(kpid_table->exit_array[i]);
                *err = ENOMEM;
                break;
            }
            // Set return value to the pid we assigned to the process
            ret_pid = i; 
            break; 
        }
        i++; 
    }

    // Check if we weren't able to find any space in the pid table, and set the error code if we coudn't
    if (i == PID_MAX + 1) {
        *err = ENPROC;
    }

    // Returns -1 if failure, or the pid we found if successful
    return ret_pid; 
}

/*
Remove a process from the pid table and destroy the process.
*/
int
pid_table_delete(pid_t pid) {
    KASSERT(kpid_table != NULL);
    KASSERT(lock_do_i_hold(kpid_table->pid_table_lock));

    // If the process identified by pid isn't null, then destroy it, and set the entry in the pid table for that pid to null.
    if (kpid_table->pid_array[pid] != NULL) {
        proc_destroy(kpid_table->pid_array[pid]);
        kpid_table->pid_array[pid] = NULL; 
    }
    // return -1 if we're trying to remove a process that doesn't exist
    else {
        return -1; 
    }

    // Return 0 on success
    return 0;
}



