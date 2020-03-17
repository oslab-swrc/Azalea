/*   MyThreads : A small, efficient, and fast threadpool implementation in C
 *   Copyright (C) 2017  Subhranil Mukherjee (https://github.com/iamsubhranil)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, version 3 of the License.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OFFLOAD_THREAD_POOL_H
#define OFFLOAD_THREAD_POOL_H

#include <stdint.h> // Standard integer
#include <pthread.h> // The thread library
#include <stdio.h> // Standard output functions in case of errors and debug
#include <stdlib.h> // Memory management functions
#include <inttypes.h> // Standard integer format specifiers

#include "atomic.h"
#include "offload_channel.h"
#include "offload_message.h"

//#define DEBUG // The debug switch

/* The main pool structure
 * 
 * To find member descriptions, see mythreads.c .
 */


/* A singly linked list of threads. This list
 * gives tremendous flexibility managing the 
 * threads at runtime.
 */
typedef struct threadList {
	pthread_t thread; // The thread object
	struct threadList *next; // Link to next thread
} thread_list_t;

/* A singly linked list of worker functions. This
 * list is implemented as a queue to manage the
 * execution in the pool.
 */
typedef struct Job {
	void (*function)(void *); // The worker function
	void *args; // Argument to the function
	struct Job *next; // Link to next Job
} job_t;

/* 
 * the args for the worker function
 */
typedef struct JobArgs {
   channel_t *ch;
   io_packet_t pkt;
} job_args_t;

/* The core pool structure. This is the only
 * user accessible structure in the API. It contains
 * all the primitives necessary to provide
 * synchronization between the threads, along with
 * dynamic management and execution control.
 */
struct threadPool {
	/* The FRONT of the thread queue in the pool.
	 * It typically points to the first thread
	 * created in the pool.
	 */
	thread_list_t * threads;

	/* The REAR of the thread queue in the pool.
	 * Points to the last, and most young thread
	 * added to the pool.
	 */
	thread_list_t * rear_threads;

	/* Number of threads in the pool. As this can
	 * grow dynamically, access and modification 
	 * of it is bounded by a mutex.
	 */
	uint64_t num_threads;

	/* The indicator which indicates the number
	 * of threads to remove. If this is equal to
	 * N, then N threads will be removed from the
	 * pool when they are idle. All threads
	 * typically check the value of this variable
	 * before executing a job, and if finds the 
	 * value >0, immediately exits.
	 */
	uint64_t remove_threads;

	/* Denotes the number of idle threads in the
	 * pool at any given instant of time. This value
	 * is used to check if all threads are idle,
	 * and thus triggering the end of job queue or
	 * the initialization of the pool, whichever
	 * applicable.
	 */
	volatile uint64_t waiting_threads;

	/* Denotes whether the pool is presently
	 * initalized or not. This variable is used to
	 * busy wait after the creation of the pool
	 * to ensure all threads are in waiting state.
	 */
	volatile uint8_t is_initialized;

	/* The main mutex for the job queue. All
	 * operations on the queue is done after locking
	 * this mutex to ensure consistency.
	 */
	pthread_mutex_t queuemutex;

	/* This mutex indicates whether a thread is
	 * presently in idle state or not, and is used
	 * in conjunction with the conditional below.
	 */
	pthread_mutex_t condmutex;

	/* Conditional to ensure conditional wait.
	 * When idle, each thread waits on this 
	 * conditional, which is signaled by various
	 * methods to indicate the wake of the thread.
	 */
	pthread_cond_t conditional;

	/* Ensures pool state. When the pool is running,
	 * this is set to 1. All the threads loop on
	 * this condition, and exits immediately when
	 * it is set to 0, which happens when the pool
	 * is destroyed.
	 */
	atomic_t run;

	/* Used to assign unique thread IDs to each
	 * running threads. It is an always incremental
	 * counter.
	 */
	uint64_t thread_id;

	/* The FRONT of the job queue, which typically
	 * points to the job to be executed next.
	 */
	job_t *FRONT;

	/* The REAR of the job queue, which points
	 * to the job last added in the pool.
	 */
	job_t *REAR;

	/* Mutex used to denote the end of the job
	 * queue, which triggers the function
	 * waitForComplete.
	 */
	pthread_mutex_t endmutex;

	/* Conditional to signal the end of the job
	 * queue.
	 */
	pthread_cond_t endconditional;
	
	/* Variable to impose and withdraw
	 * the suspend state.
	 */
	uint8_t suspend;

	/* Counter to the number of jobs
	 * present in the job queue
	 */
	atomic64_t job_count;
};

typedef struct threadPool thread_pool_t;

/* The status enum to indicate any failure.
 * 
 * These values can be compared to all the functions
 * that returns an integer, to findout the status of
 * the execution of the function.
 */
typedef enum Status{
	MEMORY_UNAVAILABLE,
	QUEUE_LOCK_FAILED,
	QUEUE_UNLOCK_FAILED,
	SIGNALLING_FAILED,
	BROADCASTING_FAILED,
	COND_WAIT_FAILED,
	THREAD_CREATION_FAILED,
	POOL_NOT_INITIALIZED,
	POOL_STOPPED,
	INVALID_NUMBER,
	WAIT_ISSUED,
	COMPLETED	
} thread_pool_status;

/* Creates a new thread pool with argument number of threads. 
 * 
 * When this method returns, and if the return value is not 
 * NULL, it is assured that all threads are initialized and 
 * in waiting state. If any thread fails to initialize, 
 * typically if the pthread_create method fails, a warning 
 * message is print on the stdout. This method also can fail
 * in case of insufficient memory, which is rare, and a NULL
 * is returned in that case.
 */
thread_pool_t * create_pool(uint64_t);

/* Waits till all the threads in the pool are finished.
 *
 * When this method returns, it is assured that all threads
 * in the pool have finished executing, and in waiting state.
 */
void wait_to_complete(thread_pool_t *);

/* Destroys the argument pool.
 *
 * This method tries to stop all threads in the pool
 * immediately, and destroys any resource that the pool has
 * used in its lifetime. However, this method will not
 * return until all threads have finished processing their
 * present work. That is, this method will not halt any
 * actively executing thread. Rather, it'll wait for the
 * present jobs to complete, and will keep the threads from
 * running any new jobs. This method then joins all the
 * threads, destroys all synchronization objects, and frees
 * any remaining jobs, finally freeing the pool itself.
 */
void destroy_pool(thread_pool_t *);

/* Add a new job to the pool.
 *
 * This method adds a new job, that is a worker function,
 * to the pool. The execution of the function is performed
 * asynchronously, however. This method only assures the
 * addition of the job to the job queue. The job queue is
 * ordered in FIFO style, i.e., for this job to execute,
 * all the jobs that has been added previously has to be
 * executed first. This method doesn't guarantee the thread
 * on which the job may execute. Rather, when its turn comes,
 * the thread which first becomes idle, executes this job.
 * When all threads are idle, any one of them wakes up and
 * executes this function asynchronously.
 */
thread_pool_status add_job_to_pool(thread_pool_t *, void (*func)(void *), void *);

/* Add some new threads to the pool.
 * 
 * This function adds specified number of new threads to the 
 * argument threadpool. When this function returns, it is 
 * ensured that a new thread has been added to the pool. 
 * However, this new thread will only come to effect if there 
 * are remainder jobs, that is the job queue is not presently 
 * empty. This new thread will not steal any running jobs 
 * from the running threads. Occasionally, this method will 
 * return some error codes, typically due to the failure of 
 * pthread_create, or for insufficient memory. These error 
 * codes can be compared using the Status enum above.
 */
thread_pool_status add_threads_to_pool(thread_pool_t *, uint64_t);

/* Suspend all currently executing threads in the pool.
 *
 * This method pauses all currently executing threads in
 * the pool. When the method call returns, it is guaranteed
 * that all threads have been suspended at appropiate
 * breakpoints. However, if a thread is presently executing,
 * it is not forcefully suspended. Rather, the call waits
 * till the thread completes the present job, and then
 * halts the thread.
 */
void suspend_pool(thread_pool_t *);

/* Resume a suspended pool.
 *
 * This method resumes a pool, aynchronously, if and only 
 * if the pool was suspended before. When the method returns,
 * it is guaranteed the all the threads of the pool will
 * wake up from suspend very soon in future. This method 
 * fails if the pool was not previously suspended.
 */
void resume_pool(thread_pool_t *);

/* Remove an existing thread from the pool.
 *
 * This function will remove one thread from the threadpool,
 * asynchronously. That is, this method will not stop any
 * active threads, rather it'll merely indicate the wish.
 * When any active thread will become idle, before becoming
 * active again the thread will check if removal is wished.
 * If it is wished, then thread will immediately exit. This
 * method can run N times to remove N threads, however it
 * has some serious consequences. If N is greater than the
 * number of threads present in the pool, say M, then all
 * M threads will be stopped. However, next (N-M) threads
 * will also immediately exit when added to the pool. If
 * all M threads are removed from the queue, then the job
 * queue will halt, and when a new thread will be added to
 * the pool, the queue will automatically resume from the
 * position where it stopped.
 */
void remove_thread_from_pool(thread_pool_t *);

/* Returns the number of pending jobs in the pool.
 *
 * This method returns the number of pending jobs in the 
 * pool, at the instant of the issue of this call. This 
 * denotes the number of jobs the pool will  finish before 
 * idlement if no new jobs are added to the pool from this
 * instant.
 */
uint64_t get_job_count(thread_pool_t *pool);

/* Returns the number of threads present in the pool.
 *
 * The number returned by this method is aware of all
 * thread addition and removal calls. Hence only the number 
 * of threads that are "active" in the pool, either by 
 * executing a worker function or in idle wait, will be
 * returned by this method.
 */
uint64_t get_thread_count(thread_pool_t *);

#endif /* OFFLOAD_THREAD_POOL_H */
