#include <types.h>
#include <lib.h>
#include <kern/fcntl.h>
#include <copyinout.h>
#include <syscall.h>
#include <proc.h>
#include <current.h>
#include <kern/errno.h>
#include <open_filetable.h>
#include <limits.h>
#include <kern/seek.h>
#include <vnode.h>
#include <stat.h>


int
sys_open(const_userptr_t filename, int flags, mode_t mode, int *retval)
{
	//filename is the pathname
	char fname [NAME_MAX];
	int *err = (int*) kmalloc(sizeof(int));

	copyinstr(filename, fname, NAME_MAX, NULL);

	// /*
	// ENXIO
	// EINVAL
	// ENOENT
	// EMFILE
	// */

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

int
sys_lseek(int fd, off_t pos, int whence, int *retval)
{
	*retval = -1; 

	if (fd >= OPEN_MAX || fd < 0 || curproc->p_open_filetable->open_files[fd] == NULL) {
		return EBADF; 
	}

	bool file_supports_seeking = VOP_ISSEEKABLE(curproc->p_open_filetable->open_files[fd]->vnode);

	if (!file_supports_seeking) {
		return ESPIPE; 
	}

	if (whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END) {
		return EINVAL; 
	}

	lock_acquire(curproc->p_open_filetable->open_files[fd]->offset_lock);
	lock_acquire(curproc->p_open_filetable->open_filetable_lock);

	if (whence == SEEK_SET) {

		off_t new_offset = pos; 

		if (new_offset < 0) {
			lock_release(curproc->p_open_filetable->open_files[fd]->offset_lock);
			lock_release(curproc->p_open_filetable->open_filetable_lock);
			return EINVAL; 
		} else {
			curproc->p_open_filetable->open_files[fd]->offset = new_offset;
			*retval = new_offset; 
		}

	} else if (whence == SEEK_CUR) {
		
		off_t new_offset = curproc->p_open_filetable->open_files[fd]->offset + pos;

		if (new_offset < 0) {
			lock_release(curproc->p_open_filetable->open_files[fd]->offset_lock);
			lock_release(curproc->p_open_filetable->open_filetable_lock);
			return EINVAL; 
		} else {
			curproc->p_open_filetable->open_files[fd]->offset = new_offset;
			*retval = new_offset; 
		}

	} else if (whence == SEEK_END) {

		/* Get the end of file */
		struct stat file_size;
		int err = VOP_STAT(curproc->p_open_filetable->open_files[fd]->vnode, &file_size);

		if (err) {
			lock_release(curproc->p_open_filetable->open_files[fd]->offset_lock);
			lock_release(curproc->p_open_filetable->open_filetable_lock);
			return err;
		}

		off_t new_offset = file_size.st_size + pos;

		if (new_offset < 0) {
			lock_release(curproc->p_open_filetable->open_files[fd]->offset_lock);
			lock_release(curproc->p_open_filetable->open_filetable_lock);
			return EINVAL; 
		} else {
			curproc->p_open_filetable->open_files[fd]->offset = new_offset;
			*retval = new_offset; 
		}

	} 

	lock_release(curproc->p_open_filetable->open_files[fd]->offset_lock);
	lock_release(curproc->p_open_filetable->open_filetable_lock);

	return 0;
}

// int
// sys_close(int fd, int *retval)
// {
// 	open_filetable_destroy(curproc->open_filetable);
	
// 	return 0;
// }

// int
// sys_dup2(int oldfd, int newfd)
// {
// 	return 0;
// }




