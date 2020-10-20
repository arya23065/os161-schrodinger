#include <types.h>
#include <lib.h>
#include <copyinout.h>
#include <syscall.h>
#include <current.h>
#include <limits.h>
#include <vnode.h>
#include <vfs.h>
// #include <size_t.h>
#include <uio.h>
#include <kern/iovec.h>

int
sys_chdir(const_userptr_t pathname, int *retval) {
    
    char pname[PATH_MAX];

	copyinstr(pathname, pname, PATH_MAX, NULL);

    int err = vfs_chdir(pname); 

    if (err != 0) {
        *retval = err; 
        return -1; 
    }
	
	return 0;
}


int
sys_getcwd(userptr_t buf, size_t buflen, int *retval) {

    // char curr_dir[buflen];

    // copyinstr(buf, curr_dir, PATH_MAX, NULL);
    // all errors are checked by called functions

    struct uio *uio_buf; 
    uio_buf = (struct uio*) kmalloc(sizeof(struct uio));

    struct iovec *iovec_buf; 
    iovec_buf = (struct iovec*) kmalloc(sizeof(struct iovec));

    iovec_buf->iov_ubase = buf; 
    iovec_buf->iov_len = buflen; 

    uio_buf->uio_iov = iovec_buf; 
    uio_buf->uio_iovcnt = 1; 
    uio_buf->uio_offset = 0; 
    uio_buf->uio_resid = buflen; 
    uio_buf->uio_segflg = UIO_USERSPACE; 
    uio_buf->uio_rw = UIO_READ; 
    uio_buf->uio_space = curproc->p_addrspace;


    int err = vfs_getcwd(uio_buf);

    if (err != 0) {
        *retval = err; 
        return -1; 
    } 

    *retval = buflen; 

    // buf = uio_buf; 

    //On success, __getcwd returns the length of the data returned. On error, -1 is returned, and errno is set according to the error encountered.
	return 0;
}
