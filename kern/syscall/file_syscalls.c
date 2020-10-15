// #include <types.h>
// #include <copyinout.h>
#include <syscall.h>
#include <unistd.h>
#include <fcntl.h>



int
sys_open(const char *filename, int flags, mode_t mode)
{
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




