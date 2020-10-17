#ifndef _OPENFILETABLE_H_
#define _OPENFILETABLE_H_

#include <types.h>
#include <limits.h>
#include <open_file.h>
// #include <synch.h>
// #include <fcntl.h>



struct open_filetable {
    // array of struct pointers - is it safe to assume this?
	struct open_file *open_files[OPEN_MAX];	
	struct lock *open_filetable_lock;
    int max_index_occupied; 
};

struct open_filetable* open_filetable_create(); 
int open_filetable_destroy(struct open_filetable *open_filetable);
int open_filetable_init(struct open_filetable *open_filetable); 
int open_filetable_add(struct open_filetable *open_filetable, char *path, int openflags, mode_t mode); 



#endif /* _OPENFILETABLE_H_ */

