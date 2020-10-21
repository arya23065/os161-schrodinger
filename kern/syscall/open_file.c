
#include <types.h>
// #include <kern/errno.h>
// #include <kern/fcntl.h>
// #include <lib.h>
// #include <uio.h>
// #include <vfs.h>
// #include <fs.h>
#include <synch.h>
// #include <vnode.h>
#include <open_file.h>
// #include <device.h>
// #include <spinlock.h>


struct open_file*
open_file_create(int status, struct vnode *vnode) {

    // KASSERT(status != NULL); 
    KASSERT(vnode != NULL); 

    struct open_file* new_file; 

    new_file = (struct open_file*) kmalloc(sizeof(struct open_file));

    if (new_file == NULL) {
        return NULL;
    }

    new_file->offset_lock = lock_create("open file lock");

    new_file->status = status; 
    new_file->offset = 0; 
    new_file->vnode = vnode;
    new_file->of_refcount = 1;

    return new_file; 
}

int
open_file_destroy(struct open_file *open_file) {
    KASSERT(open_file != NULL);

    lock_destroy(open_file->offset_lock);

    //you dont destroy the vnode cause it still needs to exist!!

    return 1; 
}

