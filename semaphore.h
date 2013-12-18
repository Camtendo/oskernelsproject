/*
 * semaphore.h
 *
 *  Created on: Dec 17, 2013
 *      Author: Sarah
 */

#ifndef SEMAPHORE_H_
#define SEMAPHORE_H_
#include "queue.h"
typedef struct my_sem_type {
	Q_type *blocking_queue;
	int count;
	int threads_waiting;
} my_sem_t;
void mysem_create( my_sem_t* sem, int initialCount );
void mysem_signal( my_sem_t* sem );
void mysem_wait( my_sem_t* sem );
void mysem_delete( my_sem_t* sem );
#endif /* SEMAPHORE_H_ */
