/**
 * threadpool.c
 *
 * This file will contain your implementation of a threadpool.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>

#include "threadpool.h"
#include "queue.c"


// State of the threadpool
typedef enum {
  RUNNING, EXITING
} poolstate;


// _threadpool is the internal threadpool structure that is
// cast to type "threadpool" before it given out to callers
typedef struct _threadpool_st {
	pthread_t      *array;

	pthread_mutex_t mutex; // a lock on the queue
	pthread_cond_t  jobPosted; // for when a job is posted
	pthread_cond_t  jobTaken; // for when a job is consumed 

	poolstate   state;
	int         threadCount;
	int         numLive;
	Queue       *q;

} _threadpool;


// Worker(consumer function) run by all the threads
void * work (void * sharedpool) {
	//fprintf(stdout,"working\n");

	// Create a pointer to the thread pool
	_threadpool *pool = (_threadpool *) sharedpool;

	// Local Vars for new jobs this thread will work on 
	dispatch_fn  myJob;
	void        *myArgs;

	// Obtain a lock on job queue read/write
	if (0 != pthread_mutex_lock(&(pool->mutex))) {
		fprintf(stderr, "nMutex lock failed!\n");
		exit(0);
	}

	// wait for a job 
	do {

		while(!isJobAvailable(pool->q)) {
//			fprintf(stdout,"Waiting...\n");
			pthread_cond_wait(&(pool->jobPosted), &(pool->mutex));
		}

		// If state has changed stop the job loop and prepare to terminate the thread
		if (pool->state == EXITING) break;
		// Get a job to do from the queue
		removeJob(pool->q, &myJob, &myArgs);

		// Allow producer to add more jobs to queue
		pthread_cond_signal(&(pool->jobTaken));


		// Yield the mutex lock for producer and other worker threads
		if (0 != pthread_mutex_unlock(&(pool->mutex))) {
			fprintf(stderr, "nMutex unlock failed!\n");
			exit(0);
		}


		/** Execute the job **/
		myJob(myArgs);


		// Re-obtain the lock on job queue read/write
		if (0 != pthread_mutex_lock(&(pool->mutex))) {
			fprintf(stderr, "nMutex lock failed!\n");
			exit(0);
		}
		//fprintf(stdout,"Job was available q->size: %d\n",pool->q->size);

	}  while(1);


	// Decrease the number of live threads
	pool->numLive--;

	// Wakes u pthe detroyer thread to keep terminating threads.
	pthread_cond_signal(&pool->jobTaken);
	// Yield the mutex lock
	if (0 != pthread_mutex_unlock(&(pool->mutex))) {
		fprintf(stderr, "nMutex unlock failed!\n");
		exit(0);
	}

//	fprintf(stdout,"Thread Dying!...\n");
	return NULL;
}


// Create a new threadpool to do work
threadpool create_threadpool(int num_threads_in_pool) {
	//fprintf(stdout,"creating\n");
	_threadpool *pool;

	// sanity check the argument
	if ((num_threads_in_pool <= 0) || (num_threads_in_pool > MAXT_IN_POOL)) return NULL;
	
	// Create the pool in memory
	pool = (_threadpool *) malloc(sizeof(_threadpool));
	if (pool == NULL) {
		fprintf(stderr, "\n\nOut of memory creating a new threadpool!\n");
		return NULL;
	}

	// initialize mutexes, cond vars and pool data 
	pthread_mutex_init(&(pool->mutex), NULL);
	pthread_cond_init(&(pool->jobPosted), NULL);
	pthread_cond_init(&(pool->jobTaken), NULL);
	pool->threadCount = num_threads_in_pool;
	pool->state = RUNNING;
	pool->numLive = 0;
	pool->q = makeQueue();

	// Make an array of threads
	//fprintf(stdout,"init array\n");
	pool->array = (pthread_t *) malloc (pool->threadCount * sizeof(pthread_t));
	//fprintf(stdout,"array initted\n");
	if (NULL == pool->array) {
		fprintf(stderr, "\n\nOut of memory allocating thread array!\n");
		free(pool);
		pool = NULL;
		return NULL;
	}

	int i;
	//fprintf(stdout,"entering for loop\n");
	for (i = 0; i < pool->threadCount; i++) {
		//fprintf(stdout,"in for loop\n");
		if (0 != pthread_create(pool->array + i, NULL, work, (void *) pool)) {
			fprintf(stderr, "\n\nThread creation failed:\n");
			exit(0);
		}
		//fprintf(stdout,"incrementing\n");
		pool->numLive++;
		//fprintf(stdout,"detaching\n");
		pthread_detach(pool->array[i]);  // Release thread memory when thread exits
		//fprintf(stdout,"detached\n");
	}


	return (threadpool) pool;
}


// Allocate a new job to the Threadpool
void dispatch(threadpool from_me, dispatch_fn dispatch_to_here, void *arg) {
//	fprintf(stdout,"dispatching\n");
	_threadpool *pool = (_threadpool *) from_me;
	if(pool != (_threadpool *) arg){

		if (pthread_mutex_lock(&pool->mutex) != 0) {
			perror("Mutex lock fail");
			exit(-1);
		}

		while(!canAddJob(pool->q)) {
			fprintf(stdout,"cant add job\n");
			pthread_cond_signal(&pool->jobPosted);
			pthread_cond_wait(&pool->jobTaken,&pool->mutex);
		}

		addJob(pool->q,dispatch_to_here,arg);
//		fprintf(stdout,"Job Added: %d\n",pool->q->size);
		pthread_cond_signal(&pool->jobPosted);
//		fprintf(stdout,"signalled jobPosted\n");		
		if (0 != pthread_mutex_unlock(&pool->mutex)) {
			perror("\n\nMutex unlock failed!:");
			exit(EXIT_FAILURE);
		}
//		fprintf(stdout,"Released Mutex\n");
	}
}

// Shut Down the Thread Pool
void destroy_threadpool(threadpool destroyme) {
	//fprintf(stdout,"destroying\n");
	_threadpool *pool = (_threadpool *) destroyme;

	if(pthread_mutex_lock(&pool->mutex) != 0) {
		perror("Failed to lock mutext");
		exit(-1);
	}

	pool->state = EXITING;
	while (pool->numLive > 0) {
		pthread_cond_signal(&pool->jobPosted);
		pthread_cond_wait(&pool->jobTaken, &pool->mutex);
	}
	free(pool->array);

	if (pthread_mutex_unlock(&pool->mutex) != 0) {
		perror("\n\nFailed to unlock mutex");
		exit(EXIT_FAILURE);
	}

	if (pthread_mutex_destroy(&pool->mutex) != 0) {
		perror("\nFailed to destroy mutex");
		exit(EXIT_FAILURE);
	}

	if (pthread_cond_destroy(&pool->jobPosted) != 0) {
		perror("\nFailed to destroy jobPosted");
		exit(EXIT_FAILURE);
	}

	if (pthread_cond_destroy(&pool->jobTaken) != 0) {
		perror("\nFailed to destroy jobTaken");
		exit(EXIT_FAILURE);
	}

	free(pool);
	pool = NULL;
	destroyme = NULL;
}
