
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



struct open_filetable*
open_filetable_create() {
    struct open_filetable* open_filetable; 

    open_filetable = (struct open_filetable*) kmalloc(sizeof(struct open_filetable));

    if (open_filetable == NULL) {
        return NULL;
    }

    open_filetable->open_filetable_lock = lock_create("open filetable lock");
    open_filetable->max_index_occupied = -1; 

    // filetable->open_files = array_create(); 
    // array_init(filetable->open_files); 

    return open_filetable; 
}


int
open_filetable_destroy(struct open_filetable *open_filetable) {
    KASSERT(open_filetable != NULL); 

    // array_destroy(filetable->open_files); 

    // idk if this needs to be done 
    // kfree(open_filetable->open_files);
    // open_filetable->open_files = NULL; 
    lock_destroy(open_filetable->open_filetable_lock);

    kfree(open_filetable); 
    return 0;
}

int
open_filetable_init(struct open_filetable *open_filetable) {
    KASSERT(open_filetable != NULL); 

    lock_acquire(open_filetable->open_filetable_lock); 

    // 0 is stdin, 1 is stdout, 2 is stderr -  These file descriptors should start out 
	// attached to the console device ("con:"), but your implementation must allow programs to use dup2() to change them to point elsewhere. 
    struct vnode *console_stdin  = NULL; 
    struct vnode *console_stdout  = NULL; 
    struct vnode *console_sterr  = NULL; 
    console_stdin = (struct vnode*)kmalloc(sizeof(struct vnode));
    console_stdout = (struct vnode*)kmalloc(sizeof(struct vnode));
    console_sterr = (struct vnode*)kmalloc(sizeof(struct vnode));

    char buf_in[32];
    strcpy(buf_in, "con:");

    /*initialising STDIN*/
    if (vfs_open(buf_in, O_RDONLY, 0, &console_stdin) != 0) {
        // vfs_close(console_stdin);
        lock_release(open_filetable->open_filetable_lock);
        return -1;
    } else {
        struct open_file *std_in; 
        std_in = open_file_create(O_RDONLY,  console_stdin);
        if (std_in == NULL) {
            lock_release(open_filetable->open_filetable_lock);
            return -1; 
        }
        open_filetable->max_index_occupied++; 
        open_filetable->open_files[open_filetable->max_index_occupied] = std_in;
    }

    char buf_out[32];
    strcpy(buf_out, "con:");

    /*initialising STDOUT*/
    if (vfs_open(buf_out, O_WRONLY, 0, &console_stdout) != 0) {
        // vfs_close(console_stdout);
        lock_release(open_filetable->open_filetable_lock);
        return -1;
    } else {
        struct open_file *std_out; 
        std_out = open_file_create(O_WRONLY,  console_stdout); 
        if (std_out == NULL) {
            lock_release(open_filetable->open_filetable_lock);
            return -1; 
        }
        open_filetable->max_index_occupied++;
        open_filetable->open_files[open_filetable->max_index_occupied] = std_out;
    }

    char buf_err[32];
    strcpy(buf_err, "con:");

    /*initialising STDERR*/
    if (vfs_open(buf_err, O_WRONLY, 0, &console_sterr) != 0) {
        // vfs_close(console_sterr);
        lock_release(open_filetable->open_filetable_lock);
        return -1;
    } else {
        struct open_file *std_err; 
        std_err = open_file_create(O_WRONLY,  console_sterr);
        if (std_err == NULL) {
            lock_release(open_filetable->open_filetable_lock);
            return -1; 
        }
        open_filetable->max_index_occupied++; 
        open_filetable->open_files[open_filetable->max_index_occupied] = std_err;
    }

    lock_release(open_filetable->open_filetable_lock); 

    return 0; 

}

/* Returns index in open_filetable of new file

*/
int 
open_filetable_add(struct open_filetable *open_filetable, char *path, int openflags, mode_t mode, int *err) {
    KASSERT(open_filetable != NULL); 

    //shouldnt have to pass in openfiletable cause there's only one!!!!!!!!!!!!!!!!
    lock_acquire(open_filetable->open_filetable_lock); 

    int ret_fd;
    struct vnode *new_vnode  = NULL; 
    new_vnode = (struct vnode*) kmalloc(sizeof(struct vnode));

    char buf[32]; 
    strcpy(buf, path);

    *err = vfs_open(buf, openflags, mode, &new_vnode);
    if (*err) {
        ret_fd = -1;
    } 
    else {
        struct open_file *new_file; 
        new_file = open_file_create(openflags,  new_vnode); 
        if (new_file == NULL) {
            ret_fd = -1;
        }
        else {
            int i = 0;
            while (i < OPEN_MAX) {
                if (open_filetable->open_files[i] == NULL) {
                    ret_fd = i;
                    open_filetable->open_files[i] = new_file;
                    break;
                }
                i++;
            }
            if (i == OPEN_MAX) {
                *err = EMFILE;
                ret_fd = -1;
            }
        }
    }

    lock_release(open_filetable->open_filetable_lock); 

    return ret_fd; 

}

int
open_filetable_remove(struct open_filetable *open_filetable, int fd, int *err) {
    KASSERT(open_filetable != NULL);
    lock_acquire(open_filetable->open_filetable_lock);
    
    int retval = 0;
    bool destroy_fd = false;
    if(fd < OPEN_MAX && fd >= 0) {
        if (open_filetable->open_files[fd] != NULL) {
            if (open_filetable->open_files[fd]->vnode->vn_refcount == 1) destroy_fd = true;
            vfs_close(open_filetable->open_files[fd]->vnode);

            if (destroy_fd) kfree(open_filetable->open_files[fd]);
            open_filetable->open_files[fd] = NULL;

        }
        else {
            *err = EBADF;
            retval = -1;
        }
    }
    else {
        *err = EBADF;
        retval = -1;
    }

    lock_release(open_filetable->open_filetable_lock);
    return retval;
}

int open_filetable_write(struct open_filetable *open_filetable, int fd, void *buf, size_t nbytes, int *err) {

    if (fd < 0 || fd >= OPEN_MAX || open_filetable->open_files[fd] == NULL) {
        *err = EBADF;
        return -1;
    }

    int perms = open_filetable->open_files[fd]->status & O_ACCMODE;
    if (perms == O_RDONLY) {
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

    uio_kinit(write_iov, write_uio, buf, nbytes, open_filetable->open_files[fd]->offset, UIO_WRITE);

    while (write_uio->uio_resid != 0) {
         *err = VOP_WRITE(open_filetable->open_files[fd]->vnode, write_uio);
         if (*err) {
            retval = -1;
            break;
        }
        retval += (nbytes - write_uio->uio_resid);
        nbytes = write_uio->uio_resid;
    }

    if (!*err)
        open_filetable->open_files[fd]->offset = write_uio->uio_offset;

    lock_release(open_filetable->open_files[fd]->offset_lock);
    lock_release(open_filetable->open_filetable_lock);

    return retval;
}

int open_filetable_read(struct open_filetable *open_filetable, int fd, void *buf, size_t nbytes, int *err) {

    if (fd < 0 || fd >= OPEN_MAX || open_filetable->open_files[fd] == NULL) {
        *err = EBADF;
        return -1;
    }

    int perms = open_filetable->open_files[fd]->status & O_ACCMODE;
    if (perms == O_WRONLY) {
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

    uio_kinit(read_iov, read_uio, buf, nbytes, open_filetable->open_files[fd]->offset, UIO_READ);

    lock_acquire(open_filetable->open_filetable_lock);
    lock_acquire(open_filetable->open_files[fd]->offset_lock);

    // while (read_uio->uio_resid != 0) {
         *err = VOP_READ(open_filetable->open_files[fd]->vnode, read_uio);
         if (*err) {
            retval = -1;
            lock_release(open_filetable->open_files[fd]->offset_lock);
            lock_release(open_filetable->open_filetable_lock);
            return retval;
        } 

        retval = nbytes; 
        // retval += (nbytes - read_uio->uio_resid);
        // nbytes = read_uio->uio_resid;

        // kprintf("%d\n", read_uio->uio_resid);
    // }

    // if (!*err)
        open_filetable->open_files[fd]->offset = read_uio->uio_offset;

    lock_release(open_filetable->open_files[fd]->offset_lock);
    lock_release(open_filetable->open_filetable_lock);

    return retval;
}

int open_filetable_dup2(struct open_filetable *open_filetable, int oldfd, int newfd, int *err) {
    int retval = newfd;

    if (oldfd < 0 || newfd < 0 || 
        oldfd >= OPEN_MAX || newfd >= OPEN_MAX ||
        open_filetable->open_files[oldfd] == NULL) {
            *err = EBADF;
            return -1;
        }

    lock_acquire(open_filetable->open_filetable_lock);

    if (open_filetable->open_files[newfd] != NULL) {
        retval = open_filetable_remove(open_filetable, newfd, err);
        if (*err) {
            lock_release(open_filetable->open_filetable_lock);
            return retval;
        }
    }

    KASSERT(open_filetable->open_files[newfd] == NULL);
    open_filetable->open_files[newfd] = open_filetable->open_files[oldfd];
    VOP_INCREF(open_filetable->open_files[newfd]->vnode);

    lock_release(open_filetable->open_filetable_lock);

    return retval;
}
