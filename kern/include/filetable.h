#ifndef _FILETABLE_H_
#define _FILETABLE_H_

#include <vnode.h>


struct filetable {
    // index of each element will be the file descriptor - first few will be aken in stdin, stdout, stderr 
	struct vnode open_files[];	
};

#endif /* _FILETABLE_H_ */

