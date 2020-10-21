#ifndef _OPENFILE_H_
#define _OPENFILE_H_

#include <types.h>
#include <lib.h>
// #include <uio.h>
// #include <vnode.h>
// #include <synch.h>


struct open_file {
	int status;	
    off_t offset; 
    struct vnode *vnode; 
    struct lock *offset_lock;
    int of_refcount;

};

struct open_file* open_file_create(int status,  struct vnode *vnode); 
int open_file_destroy(struct open_file *open_file); 



#endif /* _OPENFILE_H_ */
