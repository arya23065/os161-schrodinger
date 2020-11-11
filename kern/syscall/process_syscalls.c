#include <types.h>
#include <lib.h>
#include <kern/fcntl.h>
#include <copyinout.h>
#include <syscall.h>
#include <proc.h>
#include <current.h>
#include <kern/unistd.h>
#include <addrspace.h>
#include <pid_table.h>
#include <open_filetable.h>
#include <open_file.h>
#include <mips/trapframe.h>
#include <vnode.h>
#include <kern/errno.h>
#include <vfs.h>
#include <limits.h>
#include <kern/wait.h>

/*
Returns the pid of the current process
*/
int 
sys_getpid(pid_t *retval) {
    *retval = curproc->p_pid;  

    return 0;   // sys_getpid does not fail
}

/*
Fork to a new process. This function creates a new process as a "child"
of the process that calls it. It copies the architectural state, address space
of the parent, creates a kernel thread for the new process, and forks to that thread.
The trapframe of the new process is changed so that the return values are set correctly.
*/
int
sys_fork(struct trapframe *tf, pid_t *retval) {

    int err = 0;
    KASSERT(curproc != NULL);

    // Create new process
    struct proc *newproc = proc_create_runprogram(curproc->p_name);
	if (newproc == NULL) {
        *retval = -1;
		return ENOMEM;
	}

    pid_t pid = newproc->p_pid;

    // Copy address space of parent to address space of new process
    spinlock_acquire(&newproc->p_lock);
    err = as_copy(proc_getas(), &newproc->p_addrspace);
    spinlock_release(&newproc->p_lock);

    // Error handling. Remove the process from the PID table, destroy it, and destroy its exit info
    if (err) {
        lock_acquire(kpid_table->pid_table_lock);
        pid_table_delete(newproc->p_pid);
        cv_destroy(kpid_table->exit_array[pid]->exit_cv);
        kfree(kpid_table->exit_array[pid]);
        kfree(newproc);
        lock_release(kpid_table->pid_table_lock);
        *retval = -1;
        return err;
    }

    // Copy the file table so that both processes point to the same open files
    int i = 3;
    while (i < OPEN_MAX) {
        if (curproc->p_open_filetable->open_files[i] != NULL) {
            newproc->p_open_filetable->open_files[i] = curproc->p_open_filetable->open_files[i];
            open_file_incref(curproc->p_open_filetable->open_files[i]); 
        } 
        i++;
    }

    // Copy trapframe
    struct trapframe *new_tf = kmalloc(sizeof(struct trapframe));
    memcpy(new_tf, tf, sizeof(struct trapframe));
	
    // Error handling. Remove the process from the PID table, destroy it, and destroy its exit info
	if (err) {
        lock_acquire(kpid_table->pid_table_lock);
        pid_table_delete(newproc->p_pid);
        cv_destroy(kpid_table->exit_array[pid]->exit_cv);
        kfree(kpid_table->exit_array[pid]);
        kfree(newproc);
        lock_release(kpid_table->pid_table_lock);
        *retval = -1; 
		return err; 
	}

    // Create new thread for the new process, and go to child_fork() on the new thread
    err = thread_fork(newproc->p_name, newproc, child_fork, new_tf, 0);

    // Error handling. Remove the process from the PID table, destroy it, and destroy its exit info
    if (err) {
        lock_acquire(kpid_table->pid_table_lock);
        pid_table_delete(newproc->p_pid);
        cv_destroy(kpid_table->exit_array[pid]->exit_cv);
        kfree(kpid_table->exit_array[pid]);
        lock_release(kpid_table->pid_table_lock);

        kfree(newproc);
        kfree(new_tf);
        *retval = -1;
        return err;
    }

    // Set the return value of the parent process to be the pid of the child
    *retval = newproc->p_pid;

    return 0;
}

/*
This is the entrypoint function for thread_fork used in sys_fork(). We change the values on the trapframe
of the new process so that the program counter and return values are set correctly
*/
void child_fork(void *tf, unsigned long arg) {
    (void) arg;

    struct trapframe new_tf = *((struct trapframe*)tf);
    new_tf.tf_v0 = 0;       // Set return value to 0 for child process
    new_tf.tf_a3 = 0;
    new_tf.tf_epc += 4;     // Setting program counter so that same instruction doesn't run again

    mips_usermode(&new_tf); // Return to usermode
}

/*
Execute a program. This function copies in the arguments for the new program,
opens the file for the new program, creates a new address space, switches to
the new address space and activates it, loads the executable, defines the user stack,
copies the arguments into the stack, and returns to usermode for the execution of the 
new program
*/
int sys_execv(const_userptr_t program, const_userptr_t  args, int *retval) {

   if (program == NULL || args == NULL) {
       *retval  = -1; 
       return EFAULT;
   }

    int result = 0;

    // copyin the program name from user to kernel space
    char *progname = kmalloc(PATH_MAX);     // initialize the kernel buffer
    if (progname == NULL) {
        kfree(progname); 
        *retval = -1; 
        return ENOMEM; 
    }
    size_t progname_len;

    result = copyinstr(program, progname, PATH_MAX, &progname_len);     // copyin the program name
    if (result) {
        kfree(progname); 
        *retval = -1;
        return result;
    }

    if (progname_len == 1){
        kfree(progname); 
        *retval = -1; 
        return EINVAL;
    }

    kfree(progname); 

    char **args_input = (char**)args;

    /* To check if the args is an invalid user pointer */
    char invalid_pointer; 
    size_t invalid_pointer_len; 
    result = copyinstr((const_userptr_t)args_input, &invalid_pointer, ARG_MAX, &invalid_pointer_len); 
    if (result) {
        *retval = -1;
        return result; 
    }

    // Copying in the arguments from args array to kernel space using the method described in lecture
    char *args_strings = kmalloc(ARG_MAX); 
    size_t *args_strings_lens = kmalloc(ARG_MAX); 
    int no_of_args = 0; 
    int string_position = 0;
    int total_args_len = 0; 

    // Copy in individual arguments, and pad them so that the length is multiples of 4
    for (int i = 0; i < ARG_MAX; i++) {
        if (args_input[i] == 0) {
            break; 
        } else {
            /* The first argument is the program name */
            result = copyinstr((const_userptr_t)args_input[i], &args_strings[string_position], ARG_MAX, &args_strings_lens[i]);
            if (result) {
                kfree(args_strings);
                kfree(args_strings_lens);
                *retval = -1;
                return result;
            }

            size_t byte_padding = 0; 

            /* Determining how many bytes of padding need to be added - they must be 4 byte aligned */
            while ((byte_padding + args_strings_lens[i]) % 4 != 0 ) {
                byte_padding++; 
            }

            for (size_t j = args_strings_lens[i] + 1; j <= args_strings_lens[i] + byte_padding; j++) { 
                args_strings[string_position + args_strings_lens[i]] = 0; 
            }

            string_position += args_strings_lens[i] + byte_padding;  
            args_strings_lens[i] += byte_padding;   
            total_args_len += args_strings_lens[i];          
            no_of_args++; 
        }
    }

    // Make sure that the length of the arguments string is not more than ARG_MAX
    if (total_args_len >  ARG_MAX) {
        kfree(args_strings);
        kfree(args_strings_lens); 
        *retval = -1; 
        return E2BIG; 
    }

    struct addrspace *as_new;
    struct addrspace *as_old = curproc->p_addrspace; 
	struct vnode *v;
	vaddr_t entrypoint, stackptr;

    char *buf; 
    buf = kstrdup((char *)program);

	/* Open the file. */
	result = vfs_open(buf, O_RDONLY, 0, &v);
	if (result) {
        kfree(args_strings);
        kfree(args_strings_lens); 
        *retval = -1; 
		return result;
	}

	/* We should be a new process.*/
	KASSERT(proc_getas() != NULL);

	/* Create a new address space. */
	as_new = as_create();
	if (as_new == NULL) {
        kfree(args_strings);
        kfree(args_strings_lens); 
		vfs_close(v);
        *retval = -1; 
		return ENOMEM;
	}


	/* Switch to it and activate it. */
	as_old = proc_setas(as_new);

	as_activate();

	/* Load the executable. If it fails, switch back to the old addrspace and activate it */
	result = load_elf(v, &entrypoint);
	if (result) {
		kfree(args_strings);
        kfree(args_strings_lens); 
		vfs_close(v);
        proc_setas(as_old);
        as_activate();
        as_destroy(as_new);
		return result;
	}

	/* Done with the file now. */
	vfs_close(v);

	/* Define the user stack in the address space */
	result = as_define_stack(as_new, &stackptr);
	if (result) {
        kfree(args_strings);
        kfree(args_strings_lens); 
        proc_setas(as_old);
        as_activate();
        as_destroy(as_new);
        *retval = -1; 
		return result;
	}

    // Make space on the stack for the arguments
    stackptr -= ((no_of_args + 1) * 4 + total_args_len);

	userptr_t user_args_strings = (userptr_t)stackptr;
    char *argv_kernel[no_of_args + 1]; // + 1 for NULL at the end
    userptr_t argv = (userptr_t)stackptr;

    size_t size = 0; 
    size_t strings_pos = (no_of_args + 1) * 4; 

    userptr_t dest; 

    // Copyout the argument strings to the stack
    for (int i = 0; i < no_of_args; i++) {
        if (i == 0) {
            dest = (userptr_t)((char *)user_args_strings + strings_pos);
            result = copyoutstr(&args_strings[0], dest, (size_t) args_strings_lens[i], &size); 
        } else if (i < no_of_args) {
            strings_pos += args_strings_lens[i - 1]; 
            dest = (userptr_t)((char *)user_args_strings + strings_pos);
            result = copyoutstr(&args_strings[strings_pos - (no_of_args + 1) * 4], dest, (size_t) args_strings_lens[i], &size); 
        } 
        if (result) {
            kfree(args_strings);
            kfree(args_strings_lens); 
            proc_setas(as_old);
            as_activate();
            as_destroy(as_new);
            *retval = -1; 
            return result;
        }
        argv_kernel[i] = (char *)dest;
    }

    // Copyout from kernel space to user space
    argv_kernel[no_of_args] = NULL; 
    result = copyout(argv_kernel, argv, (no_of_args + 1)*4); 

    // Error handling
    if (result) {
        kfree(args_strings);
        kfree(args_strings_lens); 
        proc_setas(as_old);
        as_activate();
        as_destroy(as_new);
        *retval = -1; 
		return result;
    }

	/* Warp to user mode. */
	enter_new_process(no_of_args, argv, NULL, stackptr, entrypoint);

	/* enter_new_process should not return. */
	panic("enter_new_process has returned\n");

    return 0;
}

/*
Wait until the process indentified by pid exits.
*/
int sys_waitpid(pid_t pid, userptr_t status, int options, int *retval) {
    int err = 0;

    lock_acquire(kpid_table->pid_table_lock);

    // Check if the pid is valid
    if (pid <= 1 || pid > PID_MAX) {
        *retval = -1;
        lock_release(kpid_table->pid_table_lock);
        return ESRCH;
    }

    // Check if options is set correctly
    if (options != 0) {
        *retval = -1;
        lock_release(kpid_table->pid_table_lock);
        return EINVAL;
    }

    // Get exit info of the process we want to wait for
    struct p_exit_info *pei;
    pei = kpid_table->exit_array[pid];

    // Check if we are trying to wait for a process that doesn't exist
    if (pei == NULL) {
        *retval = -1;
        lock_release(kpid_table->pid_table_lock);
        return ESRCH;
    }

    // Check if we are trying to wait for the current process, or if we are trying to wait for 
    // a process that isn't our child
    if (pid == curproc->p_pid || pei->parent_pid != curproc->p_pid) {
        *retval = -1;
        lock_release(kpid_table->pid_table_lock);
        return ECHILD;
    }

    // If the process we want to wait for hasn't exited, wait
    if (!pei->has_exited) {
        cv_wait(pei->exit_cv, kpid_table->pid_table_lock);
        KASSERT(pei->has_exited);
    }

    // Store exit code in status buffer if the buffer isn't null
    if (status != NULL) {
        int exitcode = pei->exitcode; 
        err = copyout(&exitcode, status, sizeof(int));
        if (err) {
            lock_release(kpid_table->pid_table_lock);
            *retval = -1;
            return err;
        }
    }

    // Free the exit information
    KASSERT(pei->has_exited == true);
    kfree(kpid_table->exit_array[pid]);

    lock_release(kpid_table->pid_table_lock);

    *retval = pid;
    return 0; 
}

/*
Exit the current process. Store exit information if parent process hasn't exited
*/
int sys__exit(int exitcode) {

    pid_t pid = curproc->p_pid;
    struct proc* proc = curproc; 
    struct p_exit_info *pei;
    pei = kpid_table->exit_array[pid];

    KASSERT(pei != NULL);
    KASSERT(proc != kproc);

    lock_acquire(kpid_table->pid_table_lock);

    // Check if parent process still exists
    if (kpid_table->exit_array[pei->parent_pid] != NULL && kpid_table->exit_array[pei->parent_pid]->has_exited == false) {
        pei->has_exited = true;                                 // Store exit information
        pei->exitcode = _MKWAIT_EXIT(exitcode);                 // Set exit code
        cv_broadcast(pei->exit_cv, kpid_table->pid_table_lock); // Let parent know that we are exiting
    }
    // If parent has already exited, then destroy exit information
    else {
        cv_destroy(pei->exit_cv);
        kfree(pei);
    }

    // Remove current thread from current process
    proc_remthread(curthread);
    KASSERT(threadarray_num(&proc->p_threads) == 0);

    // Remove the process from the process table, and destroy the process
    pid_table_delete(proc->p_pid);

    // Add the current thread to the kernel process (This is important otherwise thread_exit() will fail)
    proc_addthread(kproc, curthread);

    lock_release(kpid_table->pid_table_lock);

    thread_exit();      // Exit
}

