
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
#include<uio.h>




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
    if (vfs_open(buf_err, O_RDONLY, 0, &console_sterr) != 0) {
        // vfs_close(console_sterr);
        lock_release(open_filetable->open_filetable_lock);
        return -1;
    } else {
        struct open_file *std_err; 
        std_err = open_file_create(O_RDONLY,  console_sterr);
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
        // vfs_close(new_vnode);
        ret_fd = -1;
    } 
    else {
        struct open_file *new_file; 
        new_file = open_file_create(openflags,  new_vnode); 
        if (new_file == NULL) {
            ret_fd = -1;
        }
        else if (open_filetable->max_index_occupied == OPEN_MAX - 1) {
            for (int i = 0; i < OPEN_MAX; i++) {
                if (open_filetable->open_files[i] == NULL) {
                    ret_fd = i;
                    open_filetable->open_files[i] = new_file;
                }
                if (i == OPEN_MAX - 1 && open_filetable->open_files[i] != NULL) {
                    *err = EMFILE;
                    ret_fd = -1;
                }
            }
        }
        else {
            open_filetable->max_index_occupied += 1;
            ret_fd = open_filetable->max_index_occupied;
            open_filetable->open_files[ret_fd] = new_file;
            
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
            // Find out if open_files[fd] should be freed if its the only reference to the open_file object.
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

int open_filetable_write(struct open_filetable *open_filetable, int fd, const void *buf, size_t nbytes, int *err) {

    if (fd < 0 || fd >= OPEN_MAX || open_filetable->open_files[fd] == NULL) {
        *err = EBADF;
        return -1;
    }

    struct iovec write_iov;
    write_iov.iov_ubase = buf;
    write_iov.iov_len = nbytes;
    return 0;
}



