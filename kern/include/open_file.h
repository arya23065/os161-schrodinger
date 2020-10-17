#ifndef _OPENFILE_H_
#define _OPENFILE_H_

#include <types.h>
#include <vnode.h>
#include <synch.h>


struct open_file {
	int status;	
    unsigned int offset; 
    struct vnode *vnode; 
    struct lock *offset_lock;

};

struct open_file* open_file_create(int status,  struct vnode *vnode); 
int open_file_destroy(struct open_file *open_file); 



#endif /* _OPENFILE_H_ */
