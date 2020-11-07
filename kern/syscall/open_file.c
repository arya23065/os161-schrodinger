
#include <types.h>
#include <synch.h>
#include <open_file.h>

/*
 * Create a new open file, given the file status and vnode. A pointer 
 * the the created open file is returned.
 */
struct open_file*
open_file_create(int status, struct vnode *vnode) {

    KASSERT(vnode != NULL); 

    struct open_file* new_file; 
    new_file = (struct open_file*) kmalloc(sizeof(struct open_file));

    if (new_file == NULL) {
        return NULL;
    }

    new_file->offset_lock = lock_create("open file lock");

    /* Initialise the values of the open file struct. A new open file will always have a reference count of 1 */
    new_file->status = status; 
    new_file->offset = 0; 
    new_file->vnode = vnode;
    new_file->of_refcount = 1;

    return new_file; 
}

/*
 * Destroy an open file. The vnode is not destroyed. 
 */
int
open_file_destroy(struct open_file *open_file) {

    KASSERT(open_file != NULL);

    lock_destroy(open_file->offset_lock);
    kfree(open_file); 
    return 0; 
}

/*
 * Increment ref count of open_file
 */
int
open_file_incref(struct open_file *open_file) {

    KASSERT(open_file != NULL);

    lock_acquire(open_file->offset_lock);
    open_file->of_refcount++;
    lock_release(open_file->offset_lock);

    return 0; 
}

