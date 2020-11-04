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

struct pid_table *pid_table_create();
int pid_table_destroy(struct pid_table *pid_table);
int pid_table_add(struct proc *proc, int *err);


#endif /* _PID_TABLE_H_ */
