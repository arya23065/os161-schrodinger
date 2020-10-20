#include <types.h>
#include <lib.h>
#include <copyinout.h>
#include <syscall.h>
#include <current.h>
#include <limits.h>
#include <vnode.h>
#include <vfs.h>

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


// int
// sys__getcwd(char *buf, size_t buflen)
// {
// 	return 0;
// }
