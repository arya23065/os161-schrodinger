#include <open_file.h>
#include <kern/fcntl.h>

struct open_file*
open_file_create(int status,  struct vnode *vnode) {

    KASSERT(status != NULL); 
    KASSERT(vnode != NULL); 

    struct open_file* open_file; 

    open_file = (open_file*) kmalloc(sizeof(struct open_file));

    if (open_file == NULL) {
                return NULL;
    }

    open_file->offset_lock = lock_create("offset lock");

    // filetable->open_files = array_create(); 
    // array_init(filetable->open_files); 

    open_file->status = status; 
    open_file->offset = 0; 
    open_file->vnode = vnode; 

    return open_file; 
}

int
open_file_destroy(struct open_file *open_file) {
    KASSERT(open_file != NULL);

    lock_destroy(open_file->offset_lock);

    //you dont destroy the vnode cause it still needs to exist!!

    return 1; 
}

