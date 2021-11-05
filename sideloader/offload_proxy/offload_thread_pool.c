// Copyright (C) 2021 Portions Copyright (c) 2021 Electronics and Telecommunications Research Institute
//
// SPDX-License-Identifier: GPL-3.0-only

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

#include <string.h>
#include "offload_thread_pool.h" // API header

/* The core function which is executed in each thread.
 * A pointer to the pool is passed as the argument,
 * which controls the flow of execution of the thread.
 */
static void *thread_executor(void *pl){
	thread_pool_t *pool = (thread_pool_t *)pl; // Get the pool
	pthread_mutex_lock(&pool->queuemutex); // Lock the mutex
	++pool->thread_id; // Get an id

#ifdef DEBUG
    uint64_t id = pool->thread_id;
#endif

	pthread_mutex_unlock(&pool->queuemutex); // Release the mutex

#ifdef DEBUG
	printf("\n[THREADPOOL:THREAD%" PRIu64 ":INFO] Starting execution loop!", id);
#endif
	//Start the core execution loop
	while(atomic_get(&pool->run)){ // run==1, we should get going
#ifdef DEBUG
		printf("\n[THREADPOOL:THREAD%" PRIu64 ":INFO] Trying to lock the mutex!", id);
#endif

		pthread_mutex_lock(&pool->queuemutex); //Lock the queue mutex

		if(pool->remove_threads>0){ // A thread is needed to be removed
#ifdef DEBUG
			printf("\n[THREADPOOL:THREAD%" PRIu64 ":INFO] Removal signalled! Exiting the execution loop!", id);
#endif
			pthread_mutex_lock(&pool->condmutex);
			pool->num_threads--;
			pthread_mutex_unlock(&pool->condmutex);
			break; // Exit the loop
		}
		job_t *present_job = pool->FRONT; // Get the first job
		if(present_job==NULL || pool->suspend){ // Queue is empty!

#ifdef DEBUG
			if(present_job==NULL)
				printf("\n[THREADPOOL:THREAD%" PRIu64 ":INFO] Queue is empty! Unlocking the mutex!", id);
			else
				printf("\n[THREADPOOL:THREAD%" PRIu64 ":INFO] Suspending thread!", id);
#endif
			pthread_mutex_unlock(&pool->queuemutex); // Unlock the mutex

			pthread_mutex_lock(&pool->condmutex); // Hold the conditional mutex
			pool->waiting_threads++; // Add yourself as a waiting thread
#ifdef DEBUG
			printf("\n[THREADPOOL:THREAD%" PRIu64 ":INFO] Waiting threads %" PRIu64 "!", id, pool->waiting_threads);
#endif
			if(!pool->suspend && pool->waiting_threads==pool->num_threads){ // All threads are idle
#ifdef DEBUG
				printf("\n[THREADPOOL:THREAD%" PRIu64 ":INFO] All threads are idle now!", id);
#endif
				if(pool->is_initialized){ // Pool is initialized, time to trigger the end conditional
#ifdef DEBUG
					printf("\n[THREADPOOL:THREAD%" PRIu64 ":INFO] Signaling endconditional!" ,id);
					fflush(stdout);
#endif
					pthread_mutex_lock(&pool->endmutex); // Lock the mutex
					pthread_cond_signal(&pool->endconditional); // Signal the end
					pthread_mutex_unlock(&pool->endmutex); // Release the mutex
#ifdef DEBUG
					printf("\n[THREADPOOL:THREAD%" PRIu64 ":INFO] Signalled any monitor!", id);
#endif
				}
				else // We are initializing the pool
					pool->is_initialized = 1; // Break the busy wait
			}



#ifdef DEBUG
			printf("\n[THREADPOOL:THREAD%" PRIu64 ":INFO] Going to conditional wait!", id);
			fflush(stdout);
#endif
			pthread_cond_wait(&pool->conditional, &pool->condmutex); // Idle wait on conditional
			
			/* Woke up! */

			if(pool->waiting_threads>0) // Unregister youself as a waiting thread
				pool->waiting_threads--;

			pthread_mutex_unlock(&pool->condmutex); // Woke up! Release the mutex

#ifdef DEBUG
			printf("\n[THREADPOOL:THREAD%" PRIu64 ":INFO] Woke up from conditional wait!", id);
#endif			
		}
		else{ // There is a job in the pool

			pool->FRONT = pool->FRONT->next; // Shift FRONT to right
			atomic64_dec(&pool->job_count); // Decrement the count
			
			if(pool->FRONT==NULL) // No jobs next
				pool->REAR = NULL; // Reset the REAR
#ifdef DEBUG
			else
				printf("\n[THREADPOOL:THREAD%" PRIu64 ":INFO] Remaining jobs : %" PRIu64, id, atomic64_read(&pool->job_count));

			printf("\n[THREADPOOL:THREAD%" PRIu64 ":INFO] Job recieved! Unlocking the mutex!", id);
#endif
			

			pthread_mutex_unlock(&pool->queuemutex); // Unlock the mutex

#ifdef DEBUG
			printf("\n[THREADPOOL:THREAD%" PRIu64 ":INFO] Executing the job now!", id);
			fflush(stdout);
#endif

			present_job->function(present_job->args); // Execute the job

#ifdef DEBUG
			printf("\n[THREADPOOL:THREAD%" PRIu64 ":INFO] Job completed! Releasing memory for the job!", id);
#endif

			free(present_job); // Release memory for the job
		}
	}

	
	if(atomic_get(&pool->run)){ // We exited, but the pool is running! It must be force removal!
#ifdef DEBUG
		printf("\n[THREADPOOL:THREAD%" PRIu64 ":INFO] Releasing the lock!", id);
#endif
		pool->remove_threads--; // Alright, I'm shutting now
		pthread_mutex_unlock(&pool->queuemutex); // We broke the loop, release the mutex now
#ifdef DEBUG
		printf("\n[THREADPOOL:THREAD%" PRIu64 ":INFO] Stopping now..", id);
		fflush(stdout);
#endif
	}
#ifdef DEBUG
	else // The pool is stopped
		printf("\n[THREADPOOL:THREAD%" PRIu64 ":INFO] Pool has been stopped! Exiting now..", id);
#endif

	pthread_exit((void *)COMPLETED); // Exit
}

/* This method adds 'threads' number of new threads
 * to the argument pool. See header for more details.
 */
#define THREADSTACK  65536

thread_pool_status add_threads_to_pool(thread_pool_t *pool, uint64_t threads){
	pthread_attr_t  attrs;
	pthread_attr_init(&attrs);
	pthread_attr_setstacksize(&attrs, THREADSTACK);


	if(pool==NULL){ // Sanity check
		printf("\n[THREADPOOL:ADD:ERROR] Pool is not initialized!");
		return POOL_NOT_INITIALIZED;
	}
	if(!atomic_get(&pool->run)){
		printf("\n[THREADPOOL:ADD:ERROR] Pool already stopped!");
		return POOL_STOPPED;
	}
	if(threads < 1){
		printf("\n[THREADPOOL:ADD:WARNING] Tried to add invalid number of threads %" PRIu64 "!", threads);
		return INVALID_NUMBER;
	}

	int temp = 0;
	thread_pool_status rc = COMPLETED;
#ifdef DEBUG
	printf("\n[THREADPOOL:ADD:INFO] Holding the condmutex..");
#endif
	pthread_mutex_lock(&pool->condmutex);
	pool->num_threads += threads; // Increment the thread count to prevent idle signal
	pthread_mutex_unlock(&pool->condmutex);
#ifdef DEBUG
	printf("\n[THREADPOOL:ADD:INFO] Speculative increment done!");
#endif
	uint64_t i = 0;
	for(i = 0;i < threads;i++){

		thread_list_t *new_thread = (thread_list_t *)malloc(sizeof(thread_list_t)); // Allocate a new thread
		new_thread->next = NULL;
		//temp = pthread_create(&new_thread->thread, NULL, thread_executor, (void *)pool); // Start the thread
		temp = pthread_create(&new_thread->thread, &attrs, thread_executor, (void *)pool); // Start the thread
		if(temp){
			printf("\n[THREADPOOL:ADD:ERROR] Unable to create thread %" PRIu64 "(error code %d)!", (i+1), temp);
			pthread_mutex_lock(&pool->condmutex);
			pool->num_threads--;
			pthread_mutex_unlock(&pool->condmutex);
			temp = 0;
			rc = THREAD_CREATION_FAILED;
		}
		else{
#ifdef DEBUG
			printf("\n[THREADPOOL:ADD:INFO] Initialized thread %" PRIu64 "!", (i+1));
#endif
			if(pool->rear_threads==NULL) // This is the first thread
				pool->threads = pool->rear_threads = new_thread;
			else // There are threads in the pool
				pool->rear_threads->next = new_thread;
			pool->rear_threads = new_thread; // This is definitely the last thread
		}
	}

	pthread_attr_destroy(&attrs);

	return rc;
}

/* This method removes one thread from the
 * argument pool. See header for more details.
 */
void remove_thread_from_pool(thread_pool_t *pool){
	if(pool==NULL || !pool->is_initialized){
		printf("\n[THREADPOOL:REM:ERROR] Pool is not initialized!");
		return;
	}
	if(!atomic_get(&pool->run)){
		printf("\n[THREADPOOL:REM:WARNING] Removing thread from a stopped pool!");
		return;
	}

#ifdef DEBUG
	printf("\n[THREADPOOL:REM:INFO] Acquiring the lock!");
#endif
	pthread_mutex_lock(&pool->queuemutex); // Lock the mutex
#ifdef DEBUG
	printf("\n[THREADPOOL:REM:INFO] Incrementing the removal count");
#endif
	pool->remove_threads++; // Indicate the willingness of removal
	pthread_mutex_unlock(&pool->queuemutex); // Unlock the mutex
#ifdef DEBUG
	printf("\n[THREADPOOL:REM:INFO] Waking up any sleeping threads!");
#endif
	pthread_mutex_lock(&pool->condmutex); // Lock the wait mutex
	pthread_cond_signal(&pool->conditional); // Signal any idle threads
	pthread_mutex_unlock(&pool->condmutex); // Release the wait mutex
#ifdef DEBUG
	printf("\n[THREADPOOL:REM:INFO] Signalling complete!");
#endif
}

/* This method creates a new thread pool containing
 * argument number of threads. See header for more
 * details.
 */

thread_pool_t * create_pool(uint64_t num_threads){
	thread_pool_t * pool = (thread_pool_t *)malloc(sizeof(thread_pool_t)); // Allocate memory for the pool
	if(pool==NULL){ // Oops!
		printf("[THREADPOOL:INIT:ERROR] Unable to allocate memory for the pool!");
		return NULL;
	}

#ifdef DEBUG
	printf("\n[THREADPOOL:INIT:INFO] Allocated %zu bytes for new pool!", sizeof(thread_pool_t));
#endif
	// Initialize members with default values
	pool->num_threads = 0; 
	pool->FRONT = NULL;
	pool->REAR = NULL;
	pool->waiting_threads = 0;
	pool->is_initialized = 0;
	pool->remove_threads = 0;
	pool->suspend = 0;
	pool->rear_threads = NULL;
	pool->threads = NULL;
	atomic64_set(&pool->job_count, 0);
    pool->thread_id = 0;

#ifdef DEBUG
	printf("\n[THREADPOOL:INIT:INFO] Initializing mutexes!");
#endif

	pthread_mutex_init(&pool->queuemutex, NULL); // Initialize queue mutex
	pthread_mutex_init(&pool->condmutex, NULL); // Initialize idle mutex
	pthread_mutex_init(&pool->endmutex, NULL); // Initialize end mutex

#ifdef DEBUG
	printf("\n[THREADPOOL:INIT:INFO] Initiliazing conditionals!");
#endif

	pthread_cond_init(&pool->endconditional, NULL); // Initialize end conditional
	pthread_cond_init(&pool->conditional, NULL); // Initialize idle conditional
	
	atomic_set(&pool->run, 1); // Start the pool

#ifdef DEBUG
	printf("\n[THREADPOOL:INIT:INFO] Successfully initialized all members of the pool!");
	printf("\n[THREADPOOL:INIT:INFO] Initializing %" PRIu64 " threads..",num_threads);
#endif
	
	if(num_threads<1){
		printf("\n[THREADPOOL:INIT:WARNING] Starting with no threads!");
		pool->is_initialized = 1;
	}
	else{
		add_threads_to_pool(pool, num_threads); // Add threads to the pool
#ifdef DEBUG
		printf("\n[THREADPOOL:INIT:INFO] Waiting for all threads to start..");
#endif
	}

	while(!pool->is_initialized); // Busy wait till the pool is initialized

#ifdef DEBUG
	printf("\n[THREADPOOL:INIT:INFO] New threadpool initialized successfully!");
#endif

	return pool;
}

/* Adds a new job to pool. See header for more
 * details.
 *
 */
thread_pool_status add_job_to_pool(thread_pool_t *pool, void (*func)(void *args), void *args){
	if(pool==NULL || !pool->is_initialized){ // Sanity check
		printf("\n[THREADPOOL:EXEC:ERROR] Pool is not initialized!");
		return POOL_NOT_INITIALIZED;
	}
	if(!atomic_get(&pool->run)){
		printf("\n[THREADPOOL:EXEC:ERROR] Trying to add a job in a stopped pool!");
		return POOL_STOPPED;
	}
	if(atomic_get(&pool->run)==2){
		printf("\n[THREADPOOL:EXEC:WARNING] Another thread is waiting for the pool to complete!");
		return WAIT_ISSUED;
	}

	job_t *new_job = (job_t *)malloc(sizeof(job_t)); // Allocate memory
	if(new_job==NULL){ // Who uses 2KB RAM nowadays?
		printf("\n[THREADPOOL:EXEC:ERROR] Unable to allocate memory for new job!");
		return MEMORY_UNAVAILABLE;
	}

#ifdef DEBUG
	printf("\n[THREADPOOL:EXEC:INFO] Allocated %zu bytes for new job!", sizeof(job_t));
#endif

	new_job->function = func; // Initialize the function
	//new_job->args = args; // Initialize the argument
	memcpy((void *) new_job->args, (void *) args, sizeof(job_args_t)); // copy the argument
	new_job->next = NULL; // Reset the link

#ifdef DEBUG
	printf("\n[THREADPOOL:EXEC:INFO] Locking the queue for insertion of the job!");
#endif

	pthread_mutex_lock(&pool->queuemutex); // Inserting the job, lock the queue

	if(pool->FRONT==NULL) // This is the first job
		pool->FRONT = pool->REAR = new_job;
	else // There are other jobs
		pool->REAR->next = new_job;
	pool->REAR = new_job; // This is the last job

	atomic64_inc(&pool->job_count); // Increment the count

#ifdef DEBUG
	printf("\n[THREADPOOL:EXEC:INFO] Inserted the job at the end of the queue!");
#endif

	if(pool->waiting_threads>0){ // There are some threads sleeping, wake'em up
#ifdef DEBUG
		printf("\n[THREADPOOL:EXEC:INFO] Signaling any idle thread!");
#endif
		pthread_mutex_lock(&pool->condmutex); // Lock the mutex
		pthread_cond_signal(&pool->conditional); // Signal the conditional
		pthread_mutex_unlock(&pool->condmutex); // Release the mutex

#ifdef DEBUG
		printf("\n[THREADPOOL:EXEC:INFO] Signaling successful!");
#endif
	}
	
	pthread_mutex_unlock(&pool->queuemutex); // Finally, release the queue

#ifdef DEBUG
	printf("\n[THREADPOOL:EXEC:INFO] Unlocked the mutex!");
#endif
	return COMPLETED;
}

/* Wait for the pool to finish executing. See header
 * for more details.
 */
void wait_to_complete(thread_pool_t *pool){
	if(pool==NULL || !pool->is_initialized){ // Sanity check
		printf("\n[THREADPOOL:WAIT:ERROR] Pool is not initialized!");
		return;
	}
	if(!atomic_get(&pool->run)){
		printf("\n[THREADPOOL:WAIT:ERROR] Pool already stopped!");
		return;
	}

	atomic_set(&pool->run, 2);
	
	pthread_mutex_lock(&pool->condmutex);
	if(pool->num_threads==pool->waiting_threads){
#ifdef DEBUG
		printf("\n[THREADPOOL:WAIT:INFO] All threads are already idle!");
#endif
		pthread_mutex_unlock(&pool->condmutex);
		atomic_set(&pool->run, 1);
		return;
	}
	pthread_mutex_unlock(&pool->condmutex);
#ifdef DEBUG
	printf("\n[THREADPOOL:WAIT:INFO] Waiting for all threads to become idle..");
#endif
	pthread_mutex_lock(&pool->endmutex); // Lock the mutex
	pthread_cond_wait(&pool->endconditional, &pool->endmutex); // Wait for end signal
	pthread_mutex_unlock(&pool->endmutex); // Unlock the mutex
#ifdef DEBUG
	printf("\n[THREADPOOL:WAIT:INFO] All threads are idle now!");
#endif
	atomic_set(&pool->run, 1);
}

/* Suspend all active threads in a pool. See header
 * for more details.
 */
void suspend_pool(thread_pool_t *pool){
	if(pool==NULL || !pool->is_initialized){ // Sanity check
		printf("\n[THREADPOOL:SUSP:ERROR] Pool is not initialized!");
		return;
	}
	if(!atomic_get(&pool->run)){ // Pool is stopped
		printf("\n[THREADPOOL:SUSP:ERROR] Pool already stopped!");
		return;
	}
	if(pool->suspend){ // Pool is already suspended
		printf("\n[THREADPOOL:SUSP:ERROR] Pool already suspended!");
		return;
	}

#ifdef DEBUG
	printf("\n[THREADPOOL:SUSP:INFO] Initiating suspend..");
#endif
	pthread_mutex_lock(&pool->queuemutex); // Lock the queue
	pool->suspend = 1; // Present the wish for suspension
	pthread_mutex_unlock(&pool->queuemutex); // Release the queue
#ifdef DEBUG
	printf("\n[THREADPOOL:SUSP:INFO] Waiting for all threads to be idle..");
	fflush(stdout);
#endif
	while(pool->waiting_threads<pool->num_threads); // Busy wait till all threads are idle
#ifdef DEBUG
	printf("\n[THREADPOOL:SUSP:INFO] Successfully suspended all threads!");
#endif
}

/* Resume a suspended pool. See header for more
 * details.
 */
void resume_pool(thread_pool_t *pool){
	if(pool==NULL || !pool->is_initialized){ // Sanity check
		printf("\n[THREADPOOL:RESM:ERROR] Pool is not initialized!");
		return;
	}
	if(!atomic_get(&pool->run)){ // Pool stopped
		printf("\n[THREADPOOL:RESM:ERROR] Pool is not running!");
		return;
	}
	if(!pool->suspend){ // Pool is not suspended
		printf("\n[THREADPOOL:RESM:WARNING] Pool is not suspended!");
		return;
	}

#ifdef DEBUG
	printf("\n[THREADPOOL:RESM:INFO] Initiating resume..");
#endif
	pthread_mutex_lock(&pool->condmutex);  // Lock the conditional
	pool->suspend = 0; // Reset the state
#ifdef DEBUG
	printf("\n[THREADPOOL:RESM:INFO] Waking up all threads..");
#endif
	pthread_cond_broadcast(&pool->conditional); // Wake up all threads
	pthread_mutex_unlock(&pool->condmutex); // Release the mutex
#ifdef DEBUG
	printf("\n[THREADPOOL:RESM:INFO] Resume complete!");
#endif
}

/* Returns number of pending jobs in the pool. See
 * header for more details
 */
uint64_t get_job_count(thread_pool_t *pool){
	return atomic64_read(&pool->job_count);
}

/* Returns the number of threads in the pool. See
 * header for more details.
 */
uint64_t get_thread_count(thread_pool_t *pool){
	return pool->num_threads;
}

/* Destroy the pool. See header for more details.
 *
 */
void destroy_pool(thread_pool_t *pool){
	if(pool==NULL || !pool->is_initialized){ // Sanity check
		printf("\n[THREADPOOL:EXIT:ERROR] Pool is not initialized!");
		return;
	}

#ifdef DEBUG
	printf("\n[THREADPOOL:EXIT:INFO] Trying to wakeup all waiting threads..");
#endif
	atomic_set(&pool->run, 0); // Stop the pool

	pthread_mutex_lock(&pool->condmutex);
	pthread_cond_broadcast(&pool->conditional); // Wake up all idle threads
	pthread_mutex_unlock(&pool->condmutex);

	int rc;
#ifdef DEBUG
	printf("\n[THREADPOOL:EXIT:INFO] Waiting for all threads to exit..");
#endif

	thread_list_t *list = pool->threads, *backup = NULL; // For travsersal

	uint64_t i = 0;
	while(list != NULL){

#ifdef DEBUG
		printf("\n[THREADPOOL:EXIT:INFO] Joining thread %" PRIu64 "..", i);
#endif

		rc = pthread_join(list->thread, NULL); //  Wait for ith thread to join
		if(rc)
			printf("\n[THREADPOOL:EXIT:WARNING] Unable to join THREAD%" PRIu64 "!", i);

#ifdef DEBUG		
		else
			printf("\n[THREADPOOL:EXIT:INFO] THREAD%" PRIu64 " joined!", i);
#endif

		backup = list;
		list = list->next; // Continue

#ifdef DEBUG
		printf("\n[THREADPOOL:EXIT:INFO] Releasing memory for THREAD%" PRIu64 "..", i);
#endif

		free(backup); // Free ith thread
		i++;
	}

#ifdef DEBUG
	printf("\n[THREADPOOL:EXIT:INFO] Destroying remaining jobs..");
#endif

	// Delete remaining jobs
	while(pool->FRONT!=NULL){
		job_t *j = pool->FRONT;
		pool->FRONT = pool->FRONT->next;
		free(j);
	}

#ifdef DEBUG
	printf("\n[THREADPOOL:EXIT:INFO] Destroying conditionals..");
#endif
	rc = pthread_cond_destroy(&pool->conditional); // Destroying idle conditional
	rc = pthread_cond_destroy(&pool->endconditional); // Destroying end conditional
	if(rc)
		printf("\n[THREADPOOL:EXIT:WARNING] Unable to destroy one or more conditionals (error code %d)!", rc);

#ifdef DEBUG
	printf("\n[THREADPOOL:EXIT:INFO] Destroying the mutexes..");
#endif

	rc = pthread_mutex_destroy(&pool->queuemutex); // Destroying queue lock
	rc = pthread_mutex_destroy(&pool->condmutex); // Destroying idle lock
	rc = pthread_mutex_destroy(&pool->endmutex); // Destroying end lock
	if(rc)
		printf("\n[THREADPOOL:EXIT:WARNING] Unable to destroy one or mutexes (error code %d)!", rc);

#ifdef DEBUG
	printf("\n[THREADPOOL:EXIT:INFO] Releasing memory for the pool..");
#endif

	free(pool); // Release the pool
#ifdef DEBUG
	printf("\n[THREADPOOL:EXIT:INFO] Pool destruction completed!");
#endif
}
