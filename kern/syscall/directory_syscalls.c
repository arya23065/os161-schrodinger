#include <types.h>
#include <lib.h>
#include <copyinout.h>
#include <syscall.h>
#include <current.h>
#include <limits.h>
#include <vnode.h>
#include <vfs.h>
#include <proc.h>
#include <uio.h>
#include <kern/iovec.h>
#include <kern/errno.h>

/*
 * chdir system call: change directory
 */
int
sys_chdir(const_userptr_t pathname, int *retval) {

    if (pathname == NULL) {
        *retval = -1; 
        return EFAULT; 
    }
    
    /* Copy the pathname into the char array pname */
    char pname[PATH_MAX];
	int copy_err = copyinstr(pathname, pname, PATH_MAX, NULL);

    if (copy_err) {
        *retval = -1; 
        return copy_err; 
    }

    int err = vfs_chdir(pname); 

    if (err != 0) {
        *retval = -1; 
        return err; 
    }

    *retval = 0;	
	return 0;
}


/*
 * _getcwd system call: get current working directory
 */
int
sys_getcwd(userptr_t buf, size_t buflen, int *retval) {

    /* Create the uio struct received by vfs_getcwd */
    struct uio *uio_buf; 
    uio_buf = (struct uio*) kmalloc(sizeof(struct uio));

    /* Create the iovec struct received by the uio struct */
    struct iovec *iovec_buf; 
    iovec_buf = (struct iovec*) kmalloc(sizeof(struct iovec));

    /* Initialising the iovec struct with len buflen */
    iovec_buf->iov_ubase = buf; 
    iovec_buf->iov_len = buflen; 

    /* Initialising the uio struct to contain user process data and for reading */
    uio_buf->uio_iov = iovec_buf; 
    uio_buf->uio_iovcnt = 1; 
    uio_buf->uio_offset = 0; 
    uio_buf->uio_resid = buflen; 
    uio_buf->uio_segflg = UIO_USERSPACE; 
    uio_buf->uio_rw = UIO_READ;
    uio_buf->uio_space = curproc->p_addrspace;


    int err = vfs_getcwd(uio_buf);

    if (err != 0) {
        *retval = -1; 
        return err; 
    } 

    *retval = buflen; 

	return 0;
}
