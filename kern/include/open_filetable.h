#ifndef _OPENFILETABLE_H_
#define _OPENFILETABLE_H_

#include <types.h>
#include <lib.h>
#include <limits.h>
#include <open_file.h>
#include <synch.h>
// #include <fcntl.h>



struct open_filetable {
    // array of struct pointers - is it safe to assume this?
	struct open_file *open_files[OPEN_MAX];	
	struct lock *open_filetable_lock;
    int max_index_occupied; 
};

struct open_filetable* open_filetable_create(void);
int open_filetable_destroy(struct open_filetable *open_filetable);
int open_filetable_init(struct open_filetable *open_filetable); 
int open_filetable_add(struct open_filetable *open_filetable, char *path, int openflags, mode_t mode, int *err); 
int open_filetable_remove(struct open_filetable *open_filetable, int fd, int *err);
int open_filetable_write(struct open_filetable *open_filetable, int fd, void *buf, size_t nbytes, int *err);
int open_filetable_read(struct open_filetable *open_filetable, int fd, void *buf, size_t nbytes, int *err);



#endif /* _OPENFILETABLE_H_ */

