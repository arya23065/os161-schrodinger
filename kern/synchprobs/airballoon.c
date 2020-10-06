/*
 * Driver code for airballoon problem
 */
#include <types.h>
#include <lib.h>
#include <thread.h>
#include <test.h>

#include <synch.h>

#define N_LORD_FLOWERKILLER 8
#define NROPES 16
static int ropes_left = NROPES;

/* Data structures for rope mappings */

/* Implement this! */
struct lock *ropes_left_lock; 

struct cv *escape_cv;
struct lock *escape_lock;
bool escaped; 

struct rope {
	struct lock *rope_lock;
	volatile int rope_stake; // ground - Marigold - -1 indicates that it is not connected -  Lord Flowerkiller only switches this 
	volatile int rope_hook; //connected to balloon - Dandelion
	volatile bool rope_severed; 
};

struct rope rope_array[NROPES];
// int num;


/* Synchronization primitives */

/* Implement this! */

/*
 * Describe your design and any invariants or locking protocols
 * that must be maintained. Explain the exit conditions. How
 * do all threads know when they are done?
 */

static
void
dandelion(void *p, unsigned long arg)
{
	(void)p;
	(void)arg;

	kprintf("Dandelion thread starting\n");
	thread_yield();
	// kprintf("%d dendelion \n", rope_array[2].rope_stake);
		// kprintf("%d dendelion num\n", num);

	/* Implement this function */
	while (ropes_left > 0) {
		int hook_index = random() % NROPES;	// generate random random balloon hook index,

		lock_acquire(rope_array[hook_index].rope_lock);

		if (rope_array[hook_index].rope_severed) {
			lock_release(rope_array[hook_index].rope_lock);
		} else {
			rope_array[hook_index].rope_hook = -1; // set to -1 to indicate severed 
			rope_array[hook_index].rope_severed = true;
			lock_acquire(ropes_left_lock);
			ropes_left--;
			lock_release(ropes_left_lock);
			kprintf("Dandelion severed rope %d\n", hook_index);
			lock_release(rope_array[hook_index].rope_lock);
		}

	}

	lock_acquire(escape_lock);
	cv_signal(escape_cv, escape_lock);
	lock_release(escape_lock);

	kprintf("Dandelion thread done\n");

	thread_exit();
}

static
void
marigold(void *p, unsigned long arg)
{
	(void)p;
	(void)arg;

	// kprintf("Marigold thread starting\n");

	/* Implement this function */
		// generate random random balloon stake index,
			// check that one end is already severed before cutting

	// thread_yield();

	/* Implement this function */
	// while (ropes_left > 0) {
	// 	int stake_index = random() % NROPES;	// generate random random balloon hook index,

	// 	lock_acquire(rope_array[stake_index].rope_lock);

	// 	if (ropes[stake_index].rope_severed) {
	// 		lock_release(rope_array[stake_index].rope_lock);
	// 	} else {
	// 		lock_acquire(ropes_left_lock);
	// 		rope_array[stake_index].rope_hook = -1; // set to -1 to indicate severed 
	// 		rope_array[stake_index].rope_severed = true;
	// 		ropes_left--;
	// 		kprintf("Marigold severed rope %d", stake_index);
	// 		lock_release(rope_array[stake_index].rope_lock);
	// 		lock_release(ropes_left_lock);
	// 	}

	// }

	// kprintf("Marigold  thread done");


}

static
void
flowerkiller(void *p, unsigned long arg)
{
	(void)p;
	(void)arg;

	// kprintf("Lord FlowerKiller thread starting\n");

	/* Implement this function */
}

static
void
balloon(void *p, unsigned long arg)
{
	(void)p;
	(void)arg;

	kprintf("Balloon thread starting\n");
	//keep checking that balloon is free
	//free memory
	lock_acquire(escape_lock); 
	cv_wait(escape_cv, escape_lock); 	// dandelion does the signalling
	lock_release(escape_lock);
	escaped = true;
	
	kprintf("Balloon freed and Prince Dandelion escapes!\n");

	kprintf("Balloon thread done\n");

	thread_exit();


	/* Implement this function */
}


// Change this function as necessary
int
airballoon(int nargs, char **args)
{

	int err = 0, i;

	(void)nargs;
	(void)args;
	(void)ropes_left;

	// intialise global struct and vairbales - have the lock before you access the rope - each rope has sa lock. diff stakes locks 

	// ropes = (struct rope*)kmalloc(NROPES * sizeof(struct rope));	// kmalloc for malloc requested by kernel 

	for (int i = 0; i < NROPES; i++){
		// rope_array[i].rope_lock = lock_create("Rope " + i);
		rope_array[i].rope_lock = lock_create("Rope");
		rope_array[i].rope_stake = i;
		rope_array[i].rope_hook = i;
		rope_array[i].rope_severed = false;
	}

	// num = 32;

	// kprintf("%d main \n", rope_array[2].rope_stake);
		// kprintf("%d main num\n", num);


	ropes_left_lock = lock_create("Ropes left lock");
	escape_lock = lock_create("Escape lock");
	escape_cv = cv_create("Escape cv");
	escaped = false;


	err = thread_fork("Marigold Thread",
			  NULL, marigold, NULL, 0);
	if(err)
		goto panic;

	err = thread_fork("Dandelion Thread",
			  NULL, dandelion, NULL, 0);
	if(err)
		goto panic;

	for (i = 0; i < N_LORD_FLOWERKILLER; i++) {
		err = thread_fork("Lord FlowerKiller Thread",
				  NULL, flowerkiller, NULL, 0);
		if(err)
			goto panic;
	}

	err = thread_fork("Air Balloon",
			  NULL, balloon, NULL, 0);
	if(err)
		goto panic;

	goto done;
panic:
	panic("airballoon: thread_fork failed: %s)\n",
	      strerror(err));

done:

	while (!escaped) {
		thread_yield();
	}

	// lock_acquire(escape_lock);
	// cv_wait(escape_cv, escape_lock);
	// lock_release(escape_lock);

	
	for (int i = 0; i < NROPES; i++) {
		lock_destroy(rope_array[i].rope_lock); 
		// ropes[i] = NULL;
	}
	// kfree(ropes);
	// rope_array = NULL

	/* To allow airballoon to be run again */
	ropes_left = NROPES;

	lock_destroy(ropes_left_lock);
	lock_destroy(escape_lock);
	cv_destroy(escape_cv);

	kprintf("Main thread done\n");

	return 0;
}
