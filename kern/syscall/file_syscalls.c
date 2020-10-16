// #include <types.h>
// #include <copyinout.h>
#include <syscall.h>
#include <unistd.h>
#include <fcntl.h>



int
sys_open(const char *filename, int flags, mode_t mode)
{
	//filename is the pathname 
	KASSERT(flags == O_RDONLY || flags == O_WRONLY || flags == O_RDWR); 

	//create a pointer to a vnode pointer and pass is to vopen, and itll open it and give it back to you 

	vfs_open(filename, flags, mode, struct vnode **ret)
	return 0;
}

int
sys_read(int fd, void *buf, size_t buflen)
{
	return 0;
}

int
sys_write(int fd, const void *buf, size_t nbytes)
{
	return 0;
}

int
sys_lseek(int fd, off_t pos, int whence)
{
	return 0;
}

int
sys_close(int fd)
{
	return 0;
}

int
sys_dup2(int oldfd, int newfd)
{
	return 0;
}




