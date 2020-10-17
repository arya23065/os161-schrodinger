#ifndef _OPEN_FILE_H_
#define _OPEN_FILE_H_

#include <vnode.h>
#include <synch.h>
#include <limits.h>



struct open_file {
	int status;	
    unsigned int offset; 
    struct vnode *vnode; 
    struct lock *offset_lock;

};

struct open_file* open_file_create(int status,  struct vnode *vnode); 
int open_file_destroy(struct open_file *open_file); 



#endif /* _OPEN_FILE_H_ */
