#ifndef _FILETABLE_H_
#define _FILETABLE_H_

#include <vnode.h>
#include <limits.h>


struct filetable {
    // index of each element will be the file descriptor - first few will be aken in stdin, stdout, stderr 
    // array of struct pointers - is it safe to assume this?
	struct vnode *open_files[PID_MAX * OPEN_MAX];	
};

struct filetable* filetable_create(); 
int filetable_destroy(struct filetable *filetable);
struct filetable* filetable_init(struct filetable *filetable); 



#endif /* _FILETABLE_H_ */

