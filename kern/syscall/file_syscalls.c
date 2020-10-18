#include <types.h>
#include <lib.h>
#include <kern/fcntl.h>
#include <copyinout.h>
#include <syscall.h>
// #include <unistd.h>
#include <proc.h>
#include <current.h>
#include <kern/errno.h>
#include <open_filetable.h>
#include <limits.h>


int
sys_open(const_userptr_t filename, int flags, mode_t mode, int *retval)
{
	//filename is the pathname
	char fname [NAME_MAX];
	int *err = (int*) kmalloc(sizeof(int));

	copyinstr(filename, fname, NAME_MAX, NULL);

	// Look at flag assertion later
	const int or_flags = O_CREAT | O_EXCL | O_TRUNC | O_APPEND;

	KASSERT(flags & O_RDONLY || flags & O_WRONLY || flags & O_RDWR); 

	//UNIVERSAL OPEN FILE TABLE HOW TF
	// open_filetable_add(filetable *open_filetable, filename, flags, mode); 

	/*
	ENXIO
	EINVAL
	ENOENT
	EMFILE
	*/

	if (((int)flags & or_flags) != (int)flags) {
		/* unknown flags were set */
		return EINVAL;
	}

	*err = 0;
	*retval = open_filetable_add(curproc->p_open_filetable, fname, flags, mode, err); 

	return *err;
}

// int
// sys_read(int fd, void *buf, size_t buflen)
// {
// 	return 0;
// }

// int
// sys_write(int fd, const void *buf, size_t nbytes)
// {
// 	return 0;
// }

// int
// sys_lseek(int fd, off_t pos, int whence)
// {
// 	return 0;
// }

// int
// sys_close(int fd)
// {
// 	return 0;
// }

// int
// sys_dup2(int oldfd, int newfd)
// {
// 	return 0;
// }




