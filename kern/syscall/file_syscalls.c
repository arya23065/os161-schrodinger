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


/*
 * Function that opens a file specified by filename.
 * Returns 0 if the operation was successful,
 * or errno if the operation was unsuccessful. *retval stores the file descriptor
 * for successful opens, or -1 for unsuccessful ones.
 */
int
sys_open(const_userptr_t filename, int flags, mode_t mode, int *retval)
{
	//filename is the pathname
	if (filename == NULL) {			// Error if filename is NULL
		*retval = -1; 
		return EFAULT; 
	}

	char fname [NAME_MAX];			// Create kernel buffer to hold filename
	int err = 0;

	err = copyinstr(filename, fname, NAME_MAX, NULL);			// Copy filename from userspace to kernel space

	if (err) {
		*retval = -1; 
		return err; 
	}

	*retval = open_filetable_add(curproc->p_open_filetable, fname, flags, mode, &err); 		// Do open operation, get errno (if there is an error) and return value

	return err;
}

/*
 * Function that reads froms a file specified by fd. We read into a kernel buffer,
 * and copyout to buf if the operation was successful. Return 0 if successful, or errno
 * if failed. *retval contains number of bytes read if successful read, or -1 if failed
 */
int
sys_read(int fd, void *buf, size_t buflen, int *retval)
{
	void *kbuf;					// Kernel buffer to read data into
	kbuf = kmalloc(buflen);

	int err = 0;
	*retval = open_filetable_read(curproc->p_open_filetable, fd, kbuf, buflen, &err);		// Do read operation

	if (err) {				// Return if there is an error
		*retval = -1;
		return err;
	}

	err = copyout(kbuf, (userptr_t) buf, buflen);		// Else, copy out read data from kernel buffer to user buffer

	if (err)			// Check if copyout is successful
		*retval = -1;

	return err;
}

/*
 * Function that writes to a file specified by fd. The data to be written is in buf,
 * and is copied in to a kernel buffer. Return 0 if successful, or errno
 * if failed. *retval contains number of bytes written if successful write, or -1 if failed
 */
int
sys_write(int fd, const void *buf, size_t nbytes, int *retval)
{
	void *kbuf;				// Kernel buffer to get data to write into file
	kbuf = kmalloc(nbytes);
	int err = copyin((const_userptr_t) buf, kbuf, nbytes);		// Copyin data from user buffer to kernel buffer
	if (err) {		// Return if error
		*retval = -1;
		return err;
	}
	err = 0;
	*retval = open_filetable_write(curproc->p_open_filetable, fd, kbuf, nbytes, &err);		// Do write operation

	return err;
}

/*
 * Function changes the offset of the file specified by fd. whence determines how the new
 * offset is determined. Return 0 on successful operation, or errno if failure. *retval
 * contains the new offset if successful operation, or -1 otherwise.
 */
int
sys_lseek(int fd, off_t pos, int whence, off_t *retval)
{

	// Check if fd is valid
	if (fd >= OPEN_MAX || fd < 0 || curproc->p_open_filetable->open_files[fd] == NULL) {
		*retval = -1; 
		return EBADF; 
	}

	// Check if ile supports changing offset
	bool file_supports_seeking = VOP_ISSEEKABLE(curproc->p_open_filetable->open_files[fd]->vnode);

	if (!file_supports_seeking) {		// Return with error if can't seek on file
		*retval = -1; 
		return ESPIPE; 
	}

	// Check if whence is set correctly, and return with error if not
	if (whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END) {
		*retval = -1; 
		return EINVAL; 
	}

	// Acquire locks
	lock_acquire(curproc->p_open_filetable->open_filetable_lock);
	lock_acquire(curproc->p_open_filetable->open_files[fd]->offset_lock);		// Offset lock is here if we have system wide open file table in the future

	if (whence == SEEK_SET) {		// Set offset to pos

		off_t new_offset = pos; 

		if (new_offset < 0) {		// Offset shouldn't be negative
			lock_release(curproc->p_open_filetable->open_files[fd]->offset_lock);
			lock_release(curproc->p_open_filetable->open_filetable_lock);
			*retval = -1; 
			return EINVAL; 			// Release locks and return with error
		} else {
			curproc->p_open_filetable->open_files[fd]->offset = new_offset;		// Change offset
			*retval = new_offset; 
		}

	} else if (whence == SEEK_CUR) {		// Set offset to curr_offset + pos
		
		off_t new_offset = curproc->p_open_filetable->open_files[fd]->offset + pos;

		if (new_offset < 0) {			// Release locks and return with error if new offset is negative
			lock_release(curproc->p_open_filetable->open_files[fd]->offset_lock);
			lock_release(curproc->p_open_filetable->open_filetable_lock);
			*retval = -1; 
			return EINVAL; 
		} else {
			curproc->p_open_filetable->open_files[fd]->offset = new_offset;

			*retval = new_offset; 
		}

	} else if (whence == SEEK_END) {		// Set offset to end-of-file

		/* Get the end of file */
		struct stat file_size;
		int err = VOP_STAT(curproc->p_open_filetable->open_files[fd]->vnode, &file_size);		// Get EOF

		if (err) {		// Release locks and return with error if can't get EOF
			lock_release(curproc->p_open_filetable->open_files[fd]->offset_lock);
			lock_release(curproc->p_open_filetable->open_filetable_lock);
			*retval = -1; 
			return err;
		}

		off_t new_offset = file_size.st_size + pos;

		if (new_offset < 0) {		// Release locks and return with error if new offset is negative
			lock_release(curproc->p_open_filetable->open_files[fd]->offset_lock);
			lock_release(curproc->p_open_filetable->open_filetable_lock);
			*retval = -1; 
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

/*
 * Function that closes a file specified by fd. Return 0 if successful, or errno otherwise.
 * *retval is 0 if successful close, or -1 otherwise.
 */
int
sys_close(int fd, int *retval)
{
	int err = 0;
	lock_acquire(curproc->p_open_filetable->open_filetable_lock);			// Acquire lock
	*retval = open_filetable_remove(curproc->p_open_filetable, fd, &err);	// Do close operation
	lock_release(curproc->p_open_filetable->open_filetable_lock);			// Release lock
	
	return err;
}

/*
 * Function that duplicates oldfd so that newfd also points to the same open file as oldfd.
 * Return 0 if successful, or errno otherwise. *retval is newfd if the operation is successful,
 * or -1 otherwise
 */
int
sys_dup2(int oldfd, int newfd, int *retval)
{
	int err = 0;
	lock_acquire(curproc->p_open_filetable->open_filetable_lock);			// Acquire lock
	*retval = open_filetable_dup2(curproc->p_open_filetable, oldfd, newfd, &err);		// Do dup2 operation
	lock_release(curproc->p_open_filetable->open_filetable_lock);			// Release lock
	return err;
}




