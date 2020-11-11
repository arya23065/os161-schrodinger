
#include <types.h>
#include <lib.h>
#include <open_filetable.h>
#include <vnode.h>
#include <kern/fcntl.h>
#include <vfs.h>
#include <synch.h>
// #include <array.h>
// #include <vfs.h>

// #include <types.h>
#include <kern/errno.h>
#include <limits.h>
// #include <lib.h>
// #include <vfs.h>
// #include <vnode.h>
//#include <kern/iovec.h>
#include <uio.h>
#include <current.h>
#include <proc.h>
#include <kern/iovec.h>


/*
 * Create an open_filetable struct. Fucntion returns pointer to new 
 * open_filetable struct if successful, or NULL otherwise.
 */
struct open_filetable*
open_filetable_create() {       // Create new open_filetable struct pointer and return it
    struct open_filetable* open_filetable; 

    open_filetable = (struct open_filetable*) kmalloc(sizeof(struct open_filetable));

    if (open_filetable == NULL) {
        return NULL;
    }

    for (int i = 0; i < OPEN_MAX; i++) {
        open_filetable->open_files[i] = NULL; 
    }

    open_filetable->open_filetable_lock = lock_create("open filetable lock");

    return open_filetable; 
}

/*
 * Destroy an open_filetable.
 */
int
open_filetable_destroy(struct open_filetable *open_filetable) {     // Destroy open_filetable struct
    KASSERT(open_filetable != NULL); 

    lock_acquire(open_filetable->open_filetable_lock);
    for (int i = 0; i < OPEN_MAX; i++) {
        int err = 0; 
        if (open_filetable->open_files[i] != NULL) {
            open_filetable_remove(open_filetable, i, &err);
            if (err) {
                lock_release(open_filetable->open_filetable_lock);
                return err;
            }
        }
        kfree(open_filetable->open_files[i]);       // Free all open_files
    }
    lock_release(open_filetable->open_filetable_lock);

    lock_destroy(open_filetable->open_filetable_lock);     // Destroy lock

    kfree(open_filetable);      // Free open_filetable
    return 0;
}

/*
 * Initializes an open_filetable by opening stdin, stdout and stderr. Return 0 if successful,
 * or -1 otherwise
 */
int
open_filetable_init(struct open_filetable *open_filetable) {        // Initialize stdin, stdout, stderr
    KASSERT(open_filetable != NULL); 

    lock_acquire(open_filetable->open_filetable_lock); 

    // 0 is stdin, 1 is stdout, 2 is stderr -  These file descriptors should start out 
	// attached to the console device ("con:")
    struct vnode *console_stdin  = NULL; 
    struct vnode *console_stdout  = NULL; 
    struct vnode *console_sterr  = NULL;                                // Create vnodes for std in, out and err
    console_stdin = (struct vnode*)kmalloc(sizeof(struct vnode));
    console_stdout = (struct vnode*)kmalloc(sizeof(struct vnode));
    console_sterr = (struct vnode*)kmalloc(sizeof(struct vnode));

    char buf_in[32];
    strcpy(buf_in, "con:");

    /*initialising STDIN*/
    if (vfs_open(buf_in, O_RDONLY, 0, &console_stdin) != 0) {
        lock_release(open_filetable->open_filetable_lock);      // return if couldn't open successfully
        return -1;
    } else {
        struct open_file *std_in; 
        std_in = open_file_create(O_RDONLY, console_stdin);     // Create new open_file for stdin with read only access
        if (std_in == NULL) {               // Return with error if couldn't initialize open_file for stdin correctly
            lock_release(open_filetable->open_filetable_lock);
            return -1; 
        }
        open_filetable->open_files[0] = std_in;             // stdin is always fd 0
    }

    char buf_out[32];
    strcpy(buf_out, "con:");

    /*initialising STDOUT*/
    if (vfs_open(buf_out, O_WRONLY, 0, &console_stdout) != 0) {     // Open with write only access
        lock_release(open_filetable->open_filetable_lock);      // Return with error if couldn't open successfully
        return -1;
    } else {
        struct open_file *std_out; 
        std_out = open_file_create(O_WRONLY,  console_stdout);      // Create new openfile for stdout
        if (std_out == NULL) {
            lock_release(open_filetable->open_filetable_lock);      // Return with error if couldn't open file
            return -1; 
        }
        open_filetable->open_files[1] = std_out;                // stdout is always fd 1
    }

    char buf_err[32];
    strcpy(buf_err, "con:");

    /*initialising STDERR*/
    if (vfs_open(buf_err, O_WRONLY, 0, &console_sterr) != 0) {      // Open with write only access
        lock_release(open_filetable->open_filetable_lock);      // Return with error if couldn't open successfully
        return -1;
    } else {
        struct open_file *std_err; 
        std_err = open_file_create(O_WRONLY,  console_sterr);       // Create new openfile for stdout
        if (std_err == NULL) {
            lock_release(open_filetable->open_filetable_lock);      // Return with error if couldn't open file
            return -1; 
        }
        open_filetable->open_files[2] = std_err;        // stdout is always fd 2
    }

    lock_release(open_filetable->open_filetable_lock); 

    return 0; 

}

/*
 * Creates a new open_file object, and initializes the vnode for the file specified by path.
 * Places new open_file into smallest free index in the open_filetable. Returns index of new open_file
 * if successful, or -1 otherwise. *err is set to 0 if successful, or errno if failed.
 */
int 
open_filetable_add(struct open_filetable *open_filetable, char *path, int openflags, mode_t mode, int *err) {
    KASSERT(open_filetable != NULL); 

    lock_acquire(open_filetable->open_filetable_lock); 

    int ret_fd;
    struct vnode *new_vnode  = NULL; 
    new_vnode = (struct vnode*) kmalloc(sizeof(struct vnode));

    char buf[32]; 
    strcpy(buf, path);

    *err = vfs_open(buf, openflags, mode, &new_vnode);      // Open new file
    if (*err) {     // Set return value to -1 if error
        ret_fd = -1;
    } 
    else {
        struct open_file *new_file; 
        new_file = open_file_create(openflags,  new_vnode); 
        if (new_file == NULL) {
            ret_fd = -1;        // Set return value to -1 if error
        }
        else {
            int i = 0;
            while (i < OPEN_MAX) {      // Look for smallest free fd
                if (open_filetable->open_files[i] == NULL) {
                    ret_fd = i;
                    open_filetable->open_files[i] = new_file;
                    break;
                }
                i++;
            }
            if (i == OPEN_MAX) {        // Set return value to -1 if all fd's are occupied and set errno
                *err = EMFILE;
                ret_fd = -1;
            }
        }
    }

    lock_release(open_filetable->open_filetable_lock); 

    return ret_fd; 

}

/*
 * Close an open_file. Destroy the open_file if there are no more references to it.
 * Returns 0 if successful, or -1 otherwise. *err is set to 0 if successful, or errno if failed.
 */
int
open_filetable_remove(struct open_filetable *open_filetable, int fd, int *err) {
    KASSERT(open_filetable != NULL);

    int retval = 0;

    if(fd < OPEN_MAX && fd >= 0) {
        if (open_filetable->open_files[fd] != NULL) {       // if fd is open, then close file
            vfs_close(open_filetable->open_files[fd]->vnode);
            open_filetable->open_files[fd]->of_refcount--;      // reduce reference count for open_file

            if (open_filetable->open_files[fd]->of_refcount == 0)
                open_file_destroy(open_filetable->open_files[fd]);          // Free open_file if there are no more references to it
            open_filetable->open_files[fd] = NULL;

        }
        else {          // Set return value to -1 if error and set errno
            *err = EBADF;
            retval = -1;
        }
    }
    else {          // Set return value to -1 if error and set errno
        *err = EBADF;
        retval = -1;
    }

    return retval;
}

/*
 * Write to the file specified by fd. The data to write is in buf. Writes upto nbytes bytes. Returns the
 * number of bytes written if successful, or -1 if failed. *err is set to 0 if successful, or errno if failed.
 */
int open_filetable_write(struct open_filetable *open_filetable, int fd, void *buf, size_t nbytes, int *err) {

    // Check if fd is valid
    if (fd < 0 || fd >= OPEN_MAX || open_filetable->open_files[fd] == NULL) {
        *err = EBADF;       // Set return value to -1 if error and set errno
        return -1;
    }

    // Check if permissions are correct
    int perms = open_filetable->open_files[fd]->status & O_ACCMODE;
    if (perms == O_RDONLY) {        // Set return value to -1 if error and set errno
        *err = EBADF;
        return -1;
    }

    int retval = 0;

    struct iovec *write_iov;
    write_iov = (struct iovec*) kmalloc(sizeof(struct iovec));
    struct uio *write_uio;
    write_uio = (struct uio*) kmalloc(sizeof(struct uio));

    lock_acquire(open_filetable->open_filetable_lock);
    lock_acquire(open_filetable->open_files[fd]->offset_lock);

    // Initialize iuo struct
    uio_kinit(write_iov, write_uio, buf, nbytes, open_filetable->open_files[fd]->offset, UIO_WRITE);

    // Write until all bytes have been written
    while (write_uio->uio_resid != 0) {
         *err = VOP_WRITE(open_filetable->open_files[fd]->vnode, write_uio);
         if (*err) {        // Set return value to -1 if error and set errno, and break out of loop
            retval = -1;
            break;
        }
        retval += (nbytes - write_uio->uio_resid);
        nbytes = write_uio->uio_resid;
    }

    if (!*err)
        open_filetable->open_files[fd]->offset = write_uio->uio_offset;     // Change offset if write successful

    lock_release(open_filetable->open_files[fd]->offset_lock);
    lock_release(open_filetable->open_filetable_lock);
    kfree(write_iov);
    kfree(write_uio);           // Release locks, free structs, and return

    return retval;
}

/*
 * Read from the file specified by fd. The data that is read is written into buf. Reads upto nbytes bytes. Returns the
 * number of bytes read if successful, or -1 if failed. *err is set to 0 if successful, or errno if failed.
 */
int open_filetable_read(struct open_filetable *open_filetable, int fd, void *buf, size_t nbytes, int *err) {

    // Check if fd is valid
    if (fd < 0 || fd >= OPEN_MAX || open_filetable->open_files[fd] == NULL) {
        *err = EBADF;       // Set return value to -1 if error and set errno
        return -1;
    }

    // Check if we have read access
    int perms = open_filetable->open_files[fd]->status & O_ACCMODE;
    if (perms == O_WRONLY) {        // Set return value to -1 if error and set errno
        *err = EBADF;
        return -1;
    }

    int retval = 0;

    struct iovec *read_iov;
    read_iov = (struct iovec*) kmalloc(sizeof(struct iovec));

    read_iov->iov_len = nbytes;
    read_iov->iov_ubase = buf;

    struct uio *read_uio;
    read_uio = (struct uio*) kmalloc(sizeof(struct uio));

    uio_kinit(read_iov, read_uio, buf, nbytes, open_filetable->open_files[fd]->offset, UIO_READ);       // Initialize uio and iov for reading

    lock_acquire(open_filetable->open_filetable_lock);
    lock_acquire(open_filetable->open_files[fd]->offset_lock);


    *err = VOP_READ(open_filetable->open_files[fd]->vnode, read_uio);       // Read
    if (*err) {         // Release locks, free structs, and return if error
        lock_release(open_filetable->open_files[fd]->offset_lock);
        lock_release(open_filetable->open_filetable_lock);
        kfree(read_iov);
        kfree(read_uio);   
        return -1;      // Set return value to -1 if error and set errno, and return after freeing structs
        } 

    retval = nbytes - read_uio->uio_resid;      // Set return value to number of bytes read

     open_filetable->open_files[fd]->offset = read_uio->uio_offset;     // Set offset

    lock_release(open_filetable->open_files[fd]->offset_lock);
    lock_release(open_filetable->open_filetable_lock);
    kfree(read_iov);
    kfree(read_uio);           // Release locks, free structs, and return

    return retval;
}

/*
 * Duplicate an open_file so that open_files[newfd] = open_files[oldfd] for open_filetable. Both
 * fd's must point to the same open_file. Return newfd if successful, or -1 otherwise. *err is set to
 * 0 if successful, or errno if failed.
 */
int open_filetable_dup2(struct open_filetable *open_filetable, int oldfd, int newfd, int *err) {
    int retval = newfd;

    // Check if oldfd and newfd are valid
    if (oldfd < 0 || newfd < 0 || 
        oldfd >= OPEN_MAX || newfd >= OPEN_MAX ||
        open_filetable->open_files[oldfd] == NULL) {
            *err = EBADF;       // Set return value to -1 if error and set errno
            return -1;
        }

    // Do nothing and return if oldfd and newfd are the same
    if (oldfd == newfd) {
        *err = 0; 
        return oldfd; 
    }

    // Close file if newfd is open
    if (open_filetable->open_files[newfd] != NULL) {
        *err = open_filetable_remove(open_filetable, newfd, err);
        if (*err) {     // Set return value to -1 if error
            return -1;
        }
    }

    KASSERT(open_filetable->open_files[newfd] == NULL);
    open_filetable->open_files[newfd] = open_filetable->open_files[oldfd];      // Set newfd and oldfd to point at same open_file
    VOP_INCREF(open_filetable->open_files[newfd]->vnode);       // Increase ref_count of vnode
    open_filetable->open_files[newfd]->of_refcount++;           // Increase ref_count of open_file

    return retval;
}
