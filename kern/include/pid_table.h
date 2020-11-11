#ifndef _PID_TABLE_H_
#define _PID_TABLE_H_

#include <types.h>
#include <spinlock.h>
#include <limits.h>
#include <proc.h>
#include <synch.h>

/*
 * PID Table structure. This stores the processes and exit information in arrays indexed by the PID for a given process
 * Also contains a lock for synchronization between threads
 */
struct pid_table
{
	struct proc *pid_array[PID_MAX + 1];
	struct lock *pid_table_lock;
	struct p_exit_info *exit_array[PID_MAX + 1];
};

/*
 * Struct for storing exit information for exit and waitpid
 */
struct p_exit_info
{
	pid_t parent_pid;
	bool has_exited;			// true if process has exited, false if not
	struct cv *exit_cv;			// Condition variable for waiting for child process to exit, and to signal parent that child has exited
	int exitcode;				// Exit code set by user after applyinf the _MKWAIT_EXIT() macro
};

/* This is the instance of the pid table structure that is accessible to the kernel for storing processes */
extern struct pid_table *kpid_table;

// PID table operations
void pid_table_init(void);
void pid_table_destroy(void);
pid_t pid_table_add(struct proc *proc, int *err);
int pid_table_delete(pid_t pid);


#endif /* _PID_TABLE_H_ */
