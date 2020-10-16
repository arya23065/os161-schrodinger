#include <open_file.h>
#include <kern/fcntl.h>

struct open_file*
open_file_create(int status, struct vnode *vnode) {

    // KASSERT(status != NULL); 
    KASSERT(vnode != NULL); 

    struct open_file* file; 

    file = (open_file*) kmalloc(sizeof(struct open_file));

    if (file == NULL) {
                return NULL;
    }

    file->offset_lock = lock_create("offset lock");

    // filetable->open_files = array_create(); 
    // array_init(filetable->open_files); 

    file->status = status; 
    file->offset = 0; 
    file->vnode = vnode; 

    return file; 
}

int
open_file_destroy(struct open_file *open_file) {
    KASSERT(open_file != NULL);

    lock_destroy(open_file->offset_lock);

    //you dont destroy the vnode cause it still needs to exist!!

    return 1; 
}

