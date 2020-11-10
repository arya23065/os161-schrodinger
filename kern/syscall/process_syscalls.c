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


int 
sys_getpid(pid_t *retval) {
    *retval = curproc->p_pid;  
    // kprintf("in getpid - the cur process' pid is %d\n", curproc->p_pid);
    return 0;   // sys_getpid does not fail
}

int
sys_fork(struct trapframe *tf, pid_t *retval) {
    // create new proc 
    // copy addrs space - struct addrspace - as copy dumbvm.c 
    int err = 0;

    lock_acquire(kpid_table->pid_table_lock);

    struct proc *newproc = kmalloc(sizeof(*newproc));
	if (newproc == NULL) {
        *retval = -1;
        lock_release(kpid_table->pid_table_lock);
		return ENOMEM;
	}

    // char* name = strcat(curproc->p_name, "_child"); 
    // newproc->p_name = kstrdup(name);
    newproc->p_name = kstrdup(curproc->p_name);
	if (newproc->p_name == NULL) {
		kfree(newproc);
        *retval = -1; 
        lock_release(kpid_table->pid_table_lock);
		return ENOMEM;
	}

	threadarray_init(&newproc->p_threads);
	spinlock_init(&newproc->p_lock);

	/* VM fields */
	// newproc->p_addrspace = NULL;

	/* VFS fields */
	newproc->p_cwd = NULL;

	/* Open filetable */
	newproc->p_open_filetable = open_filetable_create(); 

    spinlock_acquire(&newproc->p_lock);
	if (newproc->p_cwd != NULL) {
		VOP_INCREF(newproc->p_cwd);
		newproc->p_cwd = newproc->p_cwd;
	}
	spinlock_release(&newproc->p_lock);

    // newproc = proc_create_runprogram(name);

    // if (newproc == NULL) {
    //     *retval = -1; 
    //     return NULL; // not sure what error this should return 
    // }

    // copy architectural state
    // spinlock_acquire(&newproc->p_lock);
    struct addrspace *new_addrspace;
    // if (parent_addrspace == NULL) {
    //     pid_table_delete(newproc->p_pid);
	// 	kfree(newproc);
    //     *retval = -1; 
    //     return ENOMEM; 
    // } else {
    spinlock_acquire(&newproc->p_lock);
    err = as_copy(proc_getas(), &new_addrspace);
    spinlock_release(&newproc->p_lock);

    if (err) {
        // pid_table_delete(newproc->p_pid);
        proc_destroy(newproc);
        kfree(newproc);
        *retval = -1;
        lock_release(kpid_table->pid_table_lock);
        return err;
    }
    newproc->p_addrspace = new_addrspace;

    
    // newproc->p_addrspace = new_addrspace;
    // spinlock_release(&newproc->p_lock);

    // if (err) {
    //     pid_table_delete(newproc->p_pid);
    //     *retval = -1;
    //     return err;
    // }



    // Copy open file table so that each fd points to same open file
    int i = 0;
    while (i < OPEN_MAX) {
        if (curproc->p_open_filetable->open_files[i] != NULL) {
            newproc->p_open_filetable->open_files[i] = curproc->p_open_filetable->open_files[i];
            open_file_incref(curproc->p_open_filetable->open_files[i]); 
        }
        i++;
    }

    // Copy kernel thread; return to user mode
    struct trapframe *new_tf = kmalloc(sizeof(struct trapframe));
    memcpy(new_tf, tf, sizeof(struct trapframe));
    // *new_tf = *tf;

    newproc->p_pid = pid_table_add(newproc, &err); 
	
	if (err) {
		kfree(newproc);
        *retval = -1; 
        lock_release(kpid_table->pid_table_lock);
		return err; 
	}

    err = thread_fork(newproc->p_name, newproc, child_fork, new_tf, 0);

    if (err) {
        pid_table_delete(newproc->p_pid);
        kfree(newproc);
        kfree(new_tf);
        // if fails, delete all dec reference count of all open files - add function in filetable and decref in openfile 
        // free all memory malloc'd
        *retval = -1;
        lock_release(kpid_table->pid_table_lock);
        return err;
    }

    *retval = newproc->p_pid;
    lock_release(kpid_table->pid_table_lock);

    return 0;
}

void child_fork(void *tf, unsigned long arg) {
    (void) arg;

    // struct trapframe *new_tf_pointer = tf; 
    // struct trapframe new_tf = *new_tf_pointer;
    struct trapframe new_tf = *((struct trapframe*)tf);
    new_tf.tf_v0 = 0;
    new_tf.tf_a3 = 0;
    new_tf.tf_epc += 4;     // Setting program counter so that same instruction doesn't run again

    mips_usermode(&new_tf); 
}

int sys_execv(const_userptr_t program, const_userptr_t  args, int *retval) {

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

    // (void) program;
    // (void) args;
    // (void) retval;
    int result = 0;

    char *progname = kmalloc(PATH_MAX);
    if (progname == NULL) {
        *retval = -1; 
        return ENOMEM; 
    }
    size_t progname_len;

    result = copyinstr(program, progname, PATH_MAX, &progname_len);
    if (result) {
        *retval = -1;
        return ENOENT;
    }

    if (progname_len == 1){
        *retval = -1; 
        return ENOENT;
    }


    char **args_input = (char**)args;
    // char *args_copied = kmalloc(ARG_MAX);
    // size_t args_len;
    // char *args_pointers = kmalloc(ARG_MAX); 
    // size_t args_pointers_size[ARG_MAX]; 
    char *args_strings = kmalloc(ARG_MAX); 
    size_t *args_strings_lens = kmalloc(ARG_MAX); 
    // int args_pointers_max_index = 0; 


    // /* Copy in the pointers to the arguments */
    // result = copyinstr(args, args_copied, ARG_MAX, &args_len); 
    // copyinstr_result = copyinstr(args, args_pointers, ARG_MAX, args_pointers_size[i]); 
    // if (err) {
    //     *retval = -1; 
    //     return err; 
    // }
    // result = copyin(args, args_pointers, 4); 
    // kprintf("argument")

    int no_of_args = 0; 
    int string_position = 0;
    int total_args_len = 0; 

    for (int i = 0; i < ARG_MAX; i++) {
        if (args_input[i] == 0) {
            // args_strings[i] = NULL;
            break; 
        } else {
            /* The first argument is the program name */
            // args_strings[i] = args_input[i + 1]; 
            // result = copyinstr((const_userptr_t)args_input[i+1], &args_strings[string_position], ARG_MAX, &args_strings_lens[i]);
            result = copyinstr((const_userptr_t)args_input[i], &args_strings[string_position], ARG_MAX, &args_strings_lens[i]);
            if (result) {
                *retval = -1;
                return ENOENT;
            }

            // int pad = 0; 
            size_t byte_padding = 0; 
            // size_t k = 0; 

            /* Determining how many bytes of padding need to be added - they must be 4 byte aligned */
            while ((byte_padding + args_strings_lens[i]) % 4 != 0 ) {
                byte_padding++; 
            }
            // for (size_t k = args_strings_lens[i]; k % 4 == 0; k++) {
            //     byte_allignment = k - args_strings_lens[i]; 
            // }
            for (size_t j = args_strings_lens[i] + 1; j <= args_strings_lens[i] + byte_padding; j++) { 
                args_strings[string_position + args_strings_lens[i]] = 0; 
            }

            string_position += args_strings_lens[i] + byte_padding;  
            args_strings_lens[i] += byte_padding;   
            total_args_len += args_strings_lens[i];          
            no_of_args++; 
        }
    }

    if (total_args_len >  ARG_MAX) {
        //kfree everything needed 
        *retval = -1; 
        return E2BIG; 
    }

    // char* null_char = 0; 

    // /* Padding the string arguments */
    // for (int i = 0; i < no_of_args; i++) {
    //     int pad = 0; 

    //     for (int j = 0; j <= 8; j++) {
    //         if (pad == 0 && args_strings[i][j] == 0) {
    //             pad = 1; 
    //         } else if (pad == 1) {
    //         }
    //     }
    // }


    // kprintf("the first argument is %s", args_strings[0]); 


    // int args_iterator = 0; 
    // for (int i = 0; i < ARG_MAX; i++) {
    //     if (args_input[i] == NULL) {
    //         // args_pointers_max_index = i - 1;    // if argspointer max index is null there arent any arguments  
    //         break; 
    //     } else {
    //         result = copyinstr((const_userptr_t)args_input[i], &args_pointers[args_iterator], ARG_MAX, &args_pointers_size[i]); 
    //         if (result) {
    //             *retval = -1; 
    //             return result; 
    //         }
    //         args_iterator += args_pointers_size[i]; 
    //     }
    // }

    // kprintf("argument 0 is %s\n", args_input[0]); 
    // kprintf("argument 1 is %s\n", args_input[1]); 
    // kprintf("argument 2 is %s\n", args_input[2]); 
    // kprintf("argument 3 is %s\n", args_input[3]); 
    // kprintf("progname is is %s\n", progname); 


    // for (int i = 0; i < args_pointers_max_index; i++) {
    //     err = copyinstr(args_pointers[i], args_strings[i], ARG_MAX, args_strings_size[i]); 
    //     if (err) {
    //         *retval = -1; 
    //         return err; 
    //     }
    // }

    // int i = 0;

    // while (1) {
    //     err = copyin(args + i * 4, args_pointers[i], 4);
    //     if (args_pointers[i] = 0)
    //         break;
    // }

    struct addrspace *as_new;
    struct addrspace *as_old = curproc->p_addrspace; 
	struct vnode *v;
	vaddr_t entrypoint, stackptr;

    char *buf; 
    buf = kstrdup((char *)program);

	/* Open the file. */
	result = vfs_open(buf, O_RDONLY, 0, &v);
	if (result) {
        *retval = -1; 
		return result;
	}

	/* We should be a new process.*/
	KASSERT(proc_getas() != NULL);

	/* Create a new address space. */
	as_new = as_create();
	if (as_new == NULL) {
		vfs_close(v);
        *retval = -1; 
		return ENOMEM;
	}

    // /* If there is a current address space, deactivate it */
	// if (curproc_getas() != NULL) {
	// 	as_deactivate();
	// }

	/* Switch to it and activate it. */
	as_old = proc_setas(as_new);
    // if (as_old != NULL) {
	// 	as_destroy(as_old);
	// }

	as_activate();

	/* Load the executable. If it fails, switch back to the old addrspace and activate it */
	result = load_elf(v, &entrypoint);
	if (result) {
		/* p_addrspace will go away when curproc is destroyed */
		vfs_close(v);
        proc_setas(as_old);
        as_activate();
        as_destroy(as_new);
        // kfree(newname);
		return result;
	}

	/* Done with the file now. */
	vfs_close(v);

	/* Define the user stack in the address space */
	result = as_define_stack(as_new, &stackptr);
	if (result) {
		/* p_addrspace will go away when curproc is destroyed */
		return result;
	}

    // stackptr -= total_args_len;


    // stackptr -= (no_of_args * 8 + 1);
    // stackptr -= (total_args_len);

    stackptr -= ((no_of_args + 1) * 4 + total_args_len);
    // stackptr -= (total_args_len);
	userptr_t user_args_strings = (userptr_t)stackptr;
    char *argv_kernel[no_of_args + 1]; // + 1 for NULL at the end
    userptr_t argv = (userptr_t)stackptr;



    // userptr_t *user_args_strings = kmalloc(no_of_args);
    // userptr_t user_args_strings[no_of_args];


    /* Coying out pointers */
    // args_position += args_strings_lens[no_of_args]; 
    // size_t user_args_position = 0; 
    size_t size = 0; 
    size_t strings_pos = (no_of_args + 1) * 4; 

    // size_t strings_pos = 0; 
    userptr_t dest; 

    for (int i = 0; i < no_of_args; i++) {
        if (i == 0) {
            dest = (userptr_t)((char *)user_args_strings + strings_pos);
            result = copyoutstr(&args_strings[0], dest, (size_t) args_strings_lens[i], &size); 
        } else if (i < no_of_args) {
            strings_pos += args_strings_lens[i - 1]; 
            dest = (userptr_t)((char *)user_args_strings + strings_pos);
            result = copyoutstr(&args_strings[strings_pos - (no_of_args + 1) * 4], dest, (size_t) args_strings_lens[i], &size); 
        } 
        // else {
        //     char null_char = 0; 
        //     result = copyoutstr(&null_char, &user_args_strings[strings_pos], 8, &size); 
        //     strings_pos += 8; 
        // }
        if (result) {
            /* p_addrspace will go away when curproc is destroyed */
            proc_setas(as_old);
            as_activate();
            as_destroy(as_new);
            // kfree(newname);
            *retval = -1; 
            return result;
        }
        argv_kernel[i] = (char *)dest;
    }

    argv_kernel[no_of_args] = NULL; 
    result = copyout(argv_kernel, argv, (no_of_args + 1)*4); 
    char** testing = (char**) argv; 
    (void) testing; 
    if (result) {
        /* p_addrspace will go away when curproc is destroyed */
        proc_setas(as_old);
        as_activate();
        as_destroy(as_new);
        // kfree(newname);
        *retval = -1; 
        return result;
    }


    // for (int i = 0; i <= no_of_args; i++) {
    //     if (i == 0) {
    //         result = copyoutstr(&args_strings[0], &user_args_strings[strings_pos], (size_t) args_strings_lens[i], &size); 
    //     } else if (i < no_of_args) {
    //         strings_pos += args_strings_lens[i - 1]; 
    //         result = copyoutstr(&args_strings[strings_pos - no_of_args * 8], &user_args_strings[strings_pos], (size_t) args_strings_lens[i], &size); 
    //     } 
    //     // else {
    //     //     char null_char = 0; 
    //     //     result = copyoutstr(&null_char, &user_args_strings[strings_pos], 8, &size); 
    //     //     strings_pos += 8; 
    //     // }
    //     if (result) {
    //         /* p_addrspace will go away when curproc is destroyed */
    //         proc_setas(as_old);
    //         as_activate();
    //         as_destroy(as_new);
    //         // kfree(newname);
    //         *retval = -1; 
    //         return result;
    //     }
    // }

    // size_t pointers_pos = 0; 
    // strings_pos = no_of_args * 8; 
    // char **user_args = (char**)user_args_strings;
    // char *pointer_addr; 
    // userptr_t argv = (userptr_t)stackptr;
    
    // for (int i = 0; i <= no_of_args; i++) {

    //     if (i == 0) {
    //         pointer_addr = user_args[strings_pos]; 
    //     } else if (i < no_of_args) {
    //         strings_pos += args_strings_lens[i - 1];
    //         pointer_addr = user_args[strings_pos]; 
    //     } 
    //     else {
    //         char null_char = 0; 
    //         result = copyout(&null_char, &argv[pointers_pos], 8); 
    //     }

    //     result = copyout(&pointer_addr, &argv[pointers_pos], 8); 
    //     pointers_pos += 8; 
    //     // args_position += size; 

    //     if (result) {
    //         /* p_addrspace will go away when curproc is destroyed */
    //         proc_setas(as_old);
    //         as_activate();
    //         as_destroy(as_new);
    //         // kfree(newname);
    //         *retval = -1; 
    //         return result;
    //     }
    // }



	/* Warp to user mode. */
	enter_new_process(no_of_args, argv, NULL, stackptr, entrypoint);

	/* enter_new_process should not return. */
	panic("enter_new_process has returned\n");
	// return EINVAL;

    return 0;
}

int sys_waitpid(pid_t pid, userptr_t status, int options, int *retval) {
    int err = 0;

    lock_acquire(kpid_table->pid_table_lock);

    if (pid <= 1 || pid > PID_MAX || options != 0) {
        *retval = -1;
        lock_release(kpid_table->pid_table_lock);
        return ESRCH;
    }

    struct p_exit_info *pei;
    pei = kpid_table->exit_array[pid];

    if (pei == NULL) {
        *retval = -1;
        lock_release(kpid_table->pid_table_lock);
        return ESRCH;
    }

    if (pid == curproc->p_pid || pei->parent_pid != curproc->p_pid) {
        *retval = -1;
        lock_release(kpid_table->pid_table_lock);
        return ECHILD;
    }

    if (!pei->has_exited) {
        cv_wait(pei->exit_cv, kpid_table->pid_table_lock);
    }


    if (status != NULL) {
        err = copyout(&pei->exitcode, status, sizeof(int));
        if (err) {
            *retval = -1;
            lock_release(kpid_table->pid_table_lock);
            return err;
        }
    }

    KASSERT(pei->has_exited == true);
    cv_destroy(pei->exit_cv);
    kfree(pei);
    // KASSERT(kpid_table->exit_array[pid] == NULL);

    lock_release(kpid_table->pid_table_lock);

    *retval = pid;
    return 0; 
}

int sys__exit(int exitcode, int *retval) {
    (void) retval; 

    lock_acquire(kpid_table->pid_table_lock);

    struct p_exit_info *pei;
    pei = kpid_table->exit_array[curproc->p_pid];

    KASSERT(pei != NULL);
    KASSERT(curproc != kproc);

    int num = threadarray_num(&curproc->p_threads);

    for (int i = 0; i < num; i++) {
        proc_remthread(threadarray_get(&curproc->p_threads, i));
    }

    pid_t pid = curproc->p_pid;
    pid_table_delete(pid);
    KASSERT(kpid_table->pid_array[pid] == NULL);

    proc_addthread(kproc, curthread);

    if (kpid_table->exit_array[pei->parent_pid] != NULL && kpid_table->exit_array[pei->parent_pid]->has_exited == false) {
        pei->has_exited = true;
        pei->exitcode = _MKWAIT_EXIT(exitcode);
        cv_broadcast(pei->exit_cv, kpid_table->pid_table_lock);
    }
    else {
        cv_destroy(pei->exit_cv);
        kfree(pei);
    }

    lock_release(kpid_table->pid_table_lock); 

    thread_exit();
}

