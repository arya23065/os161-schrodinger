
#include <filetable.h>
#include <vnode.h>
#include <array.h>

struct filetable*
filetable_create() {
    struct filetable* filetable; 

    filetable = kmalloc(sizeof(struct filetable));

    if (filetable == NULL) {
                return NULL;
    }

    // filetable->open_files = array_create(); 
    // array_init(filetable->open_files); 

    return filetable; 
}


int
filetable_destroy(struct filetable *filetable) {
    KASSERT(filetable != NULL); 

    // array_destroy(filetable->open_files); 

    // idk if this needs to be done 
    kfree(filetable->open_files);
    filetable->open_files = NULL; 

    kfree(filetable); 
}

int
filetable_init(struct filetable *filetable) {
    KASSERT(filetable != NULL); 

    // 0 is stdin, 1 is stdout, 2 is stderr -  These file descriptors should start out 
	// attached to the console device ("con:"), but your implementation must allow programs to use dup2() to change them to point elsewhere. 

}



