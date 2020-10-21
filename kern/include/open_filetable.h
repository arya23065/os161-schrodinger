#ifndef _OPENFILETABLE_H_
#define _OPENFILETABLE_H_

#include <types.h>
#include <lib.h>
#include <limits.h>
#include <open_file.h>
#include <synch.h>
// #include <fcntl.h>



struct open_filetable {
    // array of  pointers to the file descriptors
	struct open_file *open_files[OPEN_MAX];	
	struct lock *open_filetable_lock;       // Lock for open_filetable so that operations are synchronized properly
};

struct open_filetable* open_filetable_create(void);     // Function for creating file_table
int open_filetable_destroy(struct open_filetable *open_filetable);      // Function for destroying file_table
int open_filetable_init(struct open_filetable *open_filetable);         // Function to initialize stdin, stdout, stderr
int open_filetable_add(struct open_filetable *open_filetable, char *path, int openflags, mode_t mode, int *err);    // Helper for sys_open
int open_filetable_remove(struct open_filetable *open_filetable, int fd, int *err);         // Helper for sys_close
int open_filetable_write(struct open_filetable *open_filetable, int fd, void *buf, size_t nbytes, int *err);    // Helper for sys_write
int open_filetable_read(struct open_filetable *open_filetable, int fd, void *buf, size_t nbytes, int *err);     // Helper for sys_read
int open_filetable_dup2(struct open_filetable *open_filetable, int oldfd, int newfd, int *err);                 // Helper for sys_dup2



#endif /* _OPENFILETABLE_H_ */

