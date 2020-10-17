
#include <types.h>
#include <open_filetable.h>
#include <vnode.h>
// #include <array.h>
// #include <vfs.h>



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

    lock_acquire(open_filetable->open_filetable_lock); 

    // 0 is stdin, 1 is stdout, 2 is stderr -  These file descriptors should start out 
	// attached to the console device ("con:"), but your implementation must allow programs to use dup2() to change them to point elsewhere. 
    struct vnode *console_stdin  = NULL; 
    struct vnode *console_stdout  = NULL; 
    struct vnode *console_sterr  = NULL; 
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

    /*initialising STDERR*/
    if (vfs_open("con:", O_RDONLY, 0, &console_sterr) != 0) {
        vfs_close(console_sterr);
        return -1;
    } else {
        struct open_file *new_file; 
        if (new_file = open_file_create(O_RDONLY,  console_sterr); == NULL) {
            return -1; 
        }
        open_filetable->max_index_occupied += 1; 
    }

    lock_release(open_filetable->open_filetable_lock); 

    return 0; 

}

/* Returns index in open_filetable of new file*/
int 
open_filetable_add(struct open_filetable *open_filetable, char *path, int openflags, mode_t mode) {
    KASSERT(open_filetable != NULL); 

    //shouldnt have to pass in openfiletable cause there's only one!!!!!!!!!!!!!!!!
    lock_acquire(open_filetable->open_filetable_lock); 

    struct vnode *new_vnode  = NULL; 
    new_vnode* = (vnode*)kmalloc(sizeof(vnode));

    if (vfs_open(path, openflags, mode, &new_vnode) != 0) {
        vfs_close(new_vnode);
        return -1;
    } else {
        struct open_file *new_file; 
        if (new_file = open_file_create(openflags,  new_vnode) == NULL) {
            return -1; 
        }
        open_filetable->max_index_occupied += 1; 
    }

    int return_index = open_filetable->max_index_occupied; 

    lock_require(open_filetable->open_filetable_lock); 

    return return_index; 

}



