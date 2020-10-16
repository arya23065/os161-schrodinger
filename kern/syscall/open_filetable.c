
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
    return 1;
}

int
open_filetable_init(struct filetable *open_filetable) {
    KASSERT(open_filetable != NULL); 

    // 0 is stdin, 1 is stdout, 2 is stderr -  These file descriptors should start out 
	// attached to the console device ("con:"), but your implementation must allow programs to use dup2() to change them to point elsewhere. 
    struct vnode *console  = NULL; 
    console* = (vnode*)kmalloc(sizeof(vnode));

    if (vfs_open("con:", O_RDONLY, 0, &console) != 0) {
        vfs_close(console);
        

    } else {
        struct open_file *new_file; 
        new_file = open_file_create(O_RDONLY,  console); 
        open_filetable->max_index_occupied += 1; 
    }

}



