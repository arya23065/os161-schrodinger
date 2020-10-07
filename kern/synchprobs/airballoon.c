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

struct lock *ropes_left_lock;

struct cv *escape_cv;
struct lock *escape_lock;
bool escaped;

/*Marigold, Dandelion, and Flowerkillers are action threads*/
int action_threads_left;

struct rope {
	struct lock *rope_lock;
	volatile bool rope_severed;
};

struct stake{
	struct lock *stake_lock;
	volatile int rope_index;
};

struct rope rope_array[NROPES];

/*Each element of the array indicated the rope it is connected to */
volatile int hook_array[NROPES];

/*Each element of the array corresponds to the stake, which includes the rope it is connected to through rope_index
 	within the stake struct */
struct stake stake_array[NROPES];


/* Synchronization primitives */

static
void
dandelion(void *p, unsigned long arg)
{
	(void)p;
	(void)arg;

	kprintf("Dandelion thread starting\n");

	while (ropes_left > 0) {
		int hook_index = random() % NROPES;

		/*Since the hook_array[hook_index] = hook_index, we can use hook_index directly*/
		lock_acquire(rope_array[hook_index].rope_lock);

		struct rope *rope = &(rope_array[hook_index]);
		struct lock *rope_lock = rope->rope_lock;

		if (rope->rope_severed) {
			lock_release(rope_lock);
		} 
		else {
			rope->rope_severed = true;
			lock_release(rope_lock);

			lock_acquire(ropes_left_lock);
			ropes_left--;
			lock_release(ropes_left_lock);

			kprintf("Dandelion severed rope %d\n", hook_index);
			thread_yield();
		}
	}

	/*Dandelion signals when all ropes have been cut, to wake-up the balloon thread*/
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

	while (ropes_left > 0) {

		int stake_index = random() % NROPES;

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
		} 
		else {
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


	kprintf("Lord FlowerKiller thread starting\n");

	while (ropes_left > 0) {

		int stake_index_1 = random() % NROPES;
		int stake_index_2 = random() % NROPES;

		if (stake_index_1 != stake_index_2) {

			/*In order to prevent deadlocks, stake_index_1 is always the less than stake_index_2*/
			if (stake_index_1 > stake_index_2) {
				int smaller = stake_index_2; 
				stake_index_2 = stake_index_1;
				stake_index_1 = smaller;
			}

			lock_acquire(stake_array[stake_index_1].stake_lock);
			lock_acquire(stake_array[stake_index_2].stake_lock);

			struct stake *stake_1 = &(stake_array[stake_index_1]);
			struct stake *stake_2 = &(stake_array[stake_index_2]);

			struct lock *stake_lock_1 = stake_1->stake_lock;
			struct lock *stake_lock_2 = stake_2->stake_lock;

			int rope_index_1 = stake_1->rope_index;
			int rope_index_2 = stake_2->rope_index;

			lock_acquire(rope_array[rope_index_1].rope_lock);
			lock_acquire(rope_array[rope_index_2].rope_lock);

			struct rope *rope_1 = &(rope_array[rope_index_1]);
			struct rope *rope_2 = &(rope_array[rope_index_2]);

			struct lock *rope_lock_1 = rope_1->rope_lock;
			struct lock *rope_lock_2 = rope_2->rope_lock;

			if (rope_1->rope_severed || rope_2->rope_severed){

				lock_release(rope_lock_1);
				lock_release(rope_lock_2);

				lock_release(stake_lock_1);
				lock_release(stake_lock_2);

				continue;
			}
			else {
				lock_release(rope_lock_1);
				lock_release(rope_lock_2);

				kprintf("Lord FlowerKiller switched rope %d from stake %d to stake %d\n", rope_index_1, stake_index_1, stake_index_2);
				kprintf("Lord FlowerKiller switched rope %d from stake %d to stake %d\n", rope_index_2, stake_index_2, stake_index_1);

				/*Switching the ropes connected to each stake*/
				int temp = stake_array[stake_index_1].rope_index;
				stake_array[stake_index_1].rope_index = stake_array[stake_index_2].rope_index;
				stake_array[stake_index_2].rope_index = temp;

				lock_release(stake_lock_1);
				lock_release(stake_lock_2);

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

	/*Wait for dandelion to signal when all ropes are cut*/
	lock_acquire(escape_lock);
	cv_wait(escape_cv, escape_lock);
	lock_release(escape_lock);

	while (action_threads_left != 0) {
		thread_yield();
	}

	kprintf("Balloon freed and Prince Dandelion escapes!\n");
	kprintf("Balloon thread done\n");

	escaped = true;
	thread_exit();
}


int
airballoon(int nargs, char **args)
{

	int err = 0, i;

	(void)nargs;
	(void)args;
	(void)ropes_left;

	for (int i = 0; i < NROPES; i++){

		rope_array[i].rope_lock = lock_create("Rope");
		rope_array[i].rope_severed = false;

		hook_array[i] = i;

		stake_array[i].rope_index = i;
		stake_array[i].stake_lock = lock_create("Stake");

	}

	ropes_left_lock = lock_create("Ropes left lock");
	escape_lock = lock_create("Escape lock");

	escape_cv = cv_create("Escape cv");
	escaped = false;

	/*Marigold, Dandelion, and Flowerkillers are action threads*/
	action_threads_left = 2 + N_LORD_FLOWERKILLER;

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

	for (int i = 0; i < NROPES; i++) {
		lock_destroy(rope_array[i].rope_lock);
		lock_destroy(stake_array[i].stake_lock);
	}

	/* To allow airballoon to be run again */
	ropes_left = NROPES;

	lock_destroy(ropes_left_lock);
	lock_destroy(escape_lock);
	cv_destroy(escape_cv);

	kprintf("Main thread done\n");

	return 0;
}
