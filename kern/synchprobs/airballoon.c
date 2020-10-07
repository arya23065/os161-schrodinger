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
int action_threads_left; // accessed using escape lock
bool escaped; 

struct rope {
	struct lock *rope_lock;
	// volatile int rope_stake; // ground - Marigold - -1 indicates that it is not connected -  Lord Flowerkiller only switches this 
	// volatile int rope_hook; //connected to balloon - Dandelion
	volatile bool rope_severed; 
};

struct stake{
	struct lock *stake_lock;
	volatile int rope_index;
};

struct rope rope_array[NROPES];
volatile int hook_array[NROPES];
struct stake stake_array[NROPES];



// struct lock *hook_lock;	//lock associated with hookarray


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

		struct rope *rope = &(rope_array[hook_index]); 
		struct lock *rope_lock = rope->rope_lock;

		if (rope->rope_severed) {
			lock_release(rope_lock);
		} else {
			// lock_acquire(hook_lock);
			hook_array[hook_index] = -1; // set to -1 to indicate severed - REMOVE LATER!!!!!!!!!!!!!!!!!!
			// lock_release(hook_lock);
			rope->rope_severed = true;
			lock_release(rope_lock);
			lock_acquire(ropes_left_lock);
			ropes_left--;
			lock_release(ropes_left_lock);
			kprintf("Dandelion severed rope %d\n", hook_index);
			thread_yield();
		}
	}

	lock_acquire(escape_lock);
	cv_signal(escape_cv, escape_lock);
	action_threads_left--;
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

	kprintf("Marigold thread starting\n");

	/* Implement this function */
		// generate random random balloon stake index,
			// check that one end is already severed before cutting

	thread_yield();

	/* Implement this function */
	while (ropes_left > 0) {
		int stake_index = random() % NROPES;	// generate random random balloon hook index,

		lock_acquire(stake_array[stake_index].stake_lock);

		struct stake *stake = &(stake_array[stake_index]);
		struct lock *stake_lock = stake->stake_lock;
		int rope_index = stake->rope_index;

		lock_acquire(rope_array[rope_index].rope_lock);

		struct rope *rope = &(rope_array[rope_index]); 
		struct lock *rope_lock = rope->rope_lock;

		if (rope->rope_severed) {
			lock_release(rope_lock);
			lock_release(stake_lock);
		} else {
			// lock_acquire(stake_lock);
			// stake_array[rope_index] = -1; // set to -1 to indicate severed 
			rope->rope_severed = true;
			lock_release(rope_lock);
			kprintf("Marigold severed rope %d from stake %d\n", rope_index, stake_index);
			lock_release(stake_lock);
			lock_acquire(ropes_left_lock);
			ropes_left--;
			lock_release(ropes_left_lock);
			thread_yield();
		}
	}

	lock_acquire(escape_lock);
	action_threads_left--;
	lock_release(escape_lock);

	kprintf("Marigold thread done\n");

	thread_exit();

}

static
void
flowerkiller(void *p, unsigned long arg)
{
	(void)p;
	(void)arg;

	thread_yield();

	kprintf("Lord FlowerKiller thread starting\n");

	/* Implement this function */

	thread_yield();

	//DOES FLOWERKILLER ONLY SWITCH UNSEVERED ROPES?????????????????????????????

	/* Implement this function */
	while (ropes_left > 0) {
		int stake_index_1 = random() % NROPES;	// generate random random balloon hook index,
		int stake_index_2 = random() % NROPES;	// generate random random balloon hook index,


		if (stake_index_1 != stake_index_2) {

			lock_acquire(rope_array[stake_array[stake_index_1].rope_index].rope_lock);
			lock_acquire(rope_array[stake_array[stake_index_2].rope_index].rope_lock);

			struct stake stake_1 = stake_array[stake_index_1];
			struct stake stake_2 = stake_array[stake_index_2];

			struct lock *stake_lock_1 = stake_1.stake_lock;
			struct lock *stake_lock_2 = stake_2.stake_lock;

			int rope_index_1 = stake_1.rope_index;
			int rope_index_2 = stake_2.rope_index;

			struct rope rope_1 = rope_array[rope_index_1]; 
			struct rope rope_2 = rope_array[rope_index_2]; 

			struct lock *rope_lock_1 = rope_1.rope_lock;
			struct lock *rope_lock_2 = rope_2.rope_lock;

			if (rope_1.rope_severed || rope_2.rope_severed) {
				lock_release(rope_lock_1);
				lock_release(rope_lock_2);
			} else {
				lock_release(rope_lock_1);
				lock_release(rope_lock_2);		

				lock_acquire(stake_lock_1);
				lock_acquire(stake_lock_2);
				
				kprintf("Lord FlowerKiller switched rope %d from stake %d to stake %d\n", rope_index_1, stake_index_1, stake_index_2);	

				int temp = rope_index_1;
				rope_index_1 = rope_index_2; 
				rope_index_2 = temp;

				lock_release(stake_lock_1);
				lock_release(stake_lock_2);

								// lock_release(rope_array[rope_index_1].rope_lock);
				// lock_release(rope_array[rope_index_2].rope_lock);
				thread_yield();
			}

		}
	}

	lock_acquire(escape_lock);
	action_threads_left--;
	lock_release(escape_lock);

	kprintf("Lord FlowerKiller thread done\n");


	thread_exit();
}

static
void
balloon(void *p, unsigned long arg)
{
	(void)p;
	(void)arg;

	kprintf("Balloon thread starting\n");
	//keep checking that balloon is free
	lock_acquire(escape_lock); 
	cv_wait(escape_cv, escape_lock); 	// dandelion does the signalling
	lock_release(escape_lock);
	
	// wait until all locks have been released
	while (action_threads_left != 0) {}
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
		rope_array[i].rope_severed = false;
		hook_array[i] = i;
		stake_array[i].rope_index = i;
		stake_array[i].stake_lock = lock_create("Stake");
	}

	// num = 32;

	// kprintf("%d main \n", rope_array[2].rope_stake);
		// kprintf("%d main num\n", num);


	ropes_left_lock = lock_create("Ropes left lock");
	escape_lock = lock_create("Escape lock");
	// hook_lock = lock_create("Escape lock");

	escape_cv = cv_create("Escape cv");
	escaped = false;
	// action_threads_left = 2 + N_LORD_FLOWERKILLER; // marigold, dandelion and flowerkiller
		action_threads_left = 3; // marigold, dandelion and flowerkiller


	err = thread_fork("Marigold Thread",
			  NULL, marigold, NULL, 0);
	if(err)
		goto panic;

	err = thread_fork("Dandelion Thread",
			  NULL, dandelion, NULL, 0);
	if(err)
		goto panic;

	// for (i = 0; i < N_LORD_FLOWERKILLER; i++) {
			for (i = 0; i < 1; i++) {
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

	// kprintf("reached here\n");
	for (int i = 0; i < NROPES; i++) {
		// kprintf("lock %d is held - %d\n", i, rope_array[i].rope_lock->lk_held);
		lock_destroy(rope_array[i].rope_lock); 
		lock_destroy(stake_array[i].stake_lock);
		// rope_array[i] = NULL;
	}
	// kfree(ropes);
	// rope_array = NULL

	/* To allow airballoon to be run again */
	ropes_left = NROPES;

	lock_destroy(ropes_left_lock);
	lock_destroy(escape_lock);
	// lock_destroy(hook_lock);
	cv_destroy(escape_cv);

	kprintf("Main thread done\n");

	return 0;
}
