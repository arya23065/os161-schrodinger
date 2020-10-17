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


int
sys_open(const_userptr_t filename, int flags, mode_t mode, int *retval)
// sys_open(userptr_t *filename, userptr_t flags, userptr_t mode)
{
	(void) retval; //remove this
	//filename is the pathname 
	// KASSERT(flags == O_RDONLY || flags == O_WRONLY || flags == O_RDWR); 

	const int or_flags = O_CREAT | O_EXCL | O_TRUNC | O_APPEND;

	//UNIVERSAL OPEN FILE TABLE HOW TF
	// open_filetable_add(filetable *open_filetable, filename, flags, mode); 

	if (((int)flags & or_flags) != (int)flags) {
		/* unknown flags were set */
		return EINVAL;
	}
	//copyin 

	open_filetable_add(curproc->p_open_filetable, (char*) filename, (int) flags, (mode_t) mode); 


	// if (error) {
	// 	return -1; 
	// }

	return 0;
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




