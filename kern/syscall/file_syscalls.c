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
	int err = 69;

	copyinstr(filename, fname, NAME_MAX, NULL);

	// /*
	// ENXIO
	// EINVAL
	// ENOENT
	// EMFILE
	// */

	*retval = open_filetable_add(curproc->p_open_filetable, fname, flags, mode, &err); 

	return err;
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

int
sys_close(int fd, int *retval)
{
	int err = 69;
	*retval = open_filetable_remove(curproc->p_open_filetable, fd, &err);
	
	return err;
}

// int
// sys_dup2(int oldfd, int newfd)
// {
// 	return 0;
// }




