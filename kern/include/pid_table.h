#ifndef _PID_TABLE_H_
#define _PID_TABLE_H_

#include <types.h>
#include <spinlock.h>
#include <limits.h>
#include <proc.h>

/*
 * PID Table structure.
 */
struct pid_table
{
	struct proc *pid_array[PID_MAX];
	struct lock *pid_table_lock;
};

/* This is the pid table structure for the kernel and for kernel-only threads. */
extern struct pid_table *kpid_table;

struct pid_table *pid_table_init();
void pid_table_destroy(struct pid_table *pid_table);
pid_t pid_table_add(struct proc *proc, int *err);


#endif /* _PID_TABLE_H_ */
