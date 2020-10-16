
#include <open_filetable.h>
#include <vnode.h>
#include <array.h>
// #include <fcntl.h>
#include <vfs.h>



struct open_filetable*
open_filetable_create() {
    struct open_filetable* open_filetable; 

    open_filetable = (open_filetable*) kmalloc(sizeof(struct open_filetable));

    if (open_filetable == NULL) {
                return NULL;
    }

    open_filetable->open_filetable_lock = lock_create("open filetable lock");
    open_filetable->max_index_occupied = -1; 

    // filetable->open_files = array_create(); 
    // array_init(filetable->open_files); 

    return open_filetable; 
}


int
open_filetable_destroy(struct open_filetable *open_filetable) {
    KASSERT(open_filetable != NULL); 

    // array_destroy(filetable->open_files); 

    // idk if this needs to be done 
    // kfree(open_filetable->open_files);
    // open_filetable->open_files = NULL; 
    lock_destroy(open_filetable->open_filetable_lock);

    kfree(open_filetable); 
    return 0;
}

int
open_filetable_init(struct filetable *open_filetable) {
    KASSERT(open_filetable != NULL); 

    // 0 is stdin, 1 is stdout, 2 is stderr -  These file descriptors should start out 
	// attached to the console device ("con:"), but your implementation must allow programs to use dup2() to change them to point elsewhere. 
    struct vnode *console  = NULL; 
    console_stdin* = (vnode*)kmalloc(sizeof(vnode));
    console_stdout* = (vnode*)kmalloc(sizeof(vnode));
    console_sterr* = (vnode*)kmalloc(sizeof(vnode));


    /*initialising STDIN*/
    if (vfs_open("con:", O_RDONLY, 0, &console_stdin) != 0) {
        vfs_close(console);
        return -1;
    } else {
        struct open_file *new_file; 
        if (new_file = open_file_create(O_RDONLY,  console_stdin); == NULL) {
            return -1; 
        }
        open_filetable->max_index_occupied += 1; 
    }

    /*initialising STDOUT*/
    if (vfs_open("con:", O_WRONLY, 0, &console_stdout) != 0) {
        vfs_close(console_stdout);
        return -1;
    } else {
        struct open_file *new_file; 
        if (new_file = open_file_create(O_WRONLY,  console_stdout); == NULL) {
            return -1; 
        }
        open_filetable->max_index_occupied += 1; 
    }

    /*initialising STDIN*/
    if (vfs_open("con:", O_WRONLY, 0, &console_sterr) != 0) {
        vfs_close(console_sterr);
        return -1;
    } else {
        struct open_file *new_file; 
        if (new_file = open_file_create(O_WRONLY,  console_sterr); == NULL) {
            return -1; 
        }
        open_filetable->max_index_occupied += 1; 
    }

    return 0; 

}



