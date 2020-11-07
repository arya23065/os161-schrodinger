#ifndef _OPENFILE_H_
#define _OPENFILE_H_

#include <types.h>
#include <lib.h>

/*
 * Open file struct
 * 
 * Each process' open filetable points to open files. 
 * The file descriptors are included in the open filetable. 
 * 
 * This struct contains information on the status of the open file, the offset
 * a pointer to the vnode, reference counters and a lock for the offset. 
 * 
 */
struct open_file {
	int status;	
    off_t offset; 
    struct vnode *vnode; 
    struct lock *offset_lock;
    int of_refcount;

};

/*
 * Functions to create and destroy open files
 */
struct open_file* open_file_create(int status,  struct vnode *vnode); 
int open_file_destroy(struct open_file *open_file); 
int open_file_incref(struct open_file *open_file); 



#endif /* _OPENFILE_H_ */
