/*
 * semaphore.c
 *
 *  Created on: Dec 17, 2013
 *      Author:
 */
#include <stdio.h>
#include <stdlib.h>
#include "semaphore.h"
#include "thread_handler.h"
#define SEM_MAX 10000
/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
/* The two macros are extremely useful by turnning on/off interrupts when atomicity is required */
#define DISABLE_INTERRUPTS() {  \
    asm("wrctl status, zero");  \
}

#define ENABLE_INTERRUPTS() {   \
    asm("movi et, 1");          \
    asm("wrctl status, et");    \
}

// It returns the starting address a semaphore variable.
// You can use malloc() to allocate memory space.
void mysem_create( my_sem_t* semaphore, int initialCount ) {
	semaphore->count = initialCount;
	semaphore->threads_waiting = 0;
	Q_type *queue = (Q_type *) malloc(sizeof(Q_type));
	queue->head = NULL;
	queue->tail = NULL;
	queue->size = 0;
	semaphore->blocking_queue = queue;
	printf("Semaphore created with initial count: %d\n", semaphore->count);
}

// It performs the semaphore's signal operation.
void mysem_signal( my_sem_t* sem ) {
	DISABLE_INTERRUPTS();
	sem->count = sem->count + 1;

	printf("Signal semaphore. Updated count: %d\n", sem->count);
	if (sem->count <= 0) {//<= or >=
		if (sem->threads_waiting > 0){
			tcb *blocked_node = (tcb *) dequeueFromQueue(sem->blocking_queue);
			if (blocked_node != NULL) {
				blocked_node->state = READY;
				enqueue((void *)blocked_node);
				sem->threads_waiting = sem->threads_waiting - 1;
				printf("Unblocked: %u\nThreads waiting: %d\n",
						blocked_node->tid,
						sem->threads_waiting);
			}
		}
	}
	ENABLE_INTERRUPTS();
}

// It performs the semaphore's wait operation.
void mysem_wait( my_sem_t* sem ) {
	DISABLE_INTERRUPTS();
	sem->count = sem->count - 1;
	tcb *current_running_thread = getCurrentRunningThread();
	printf("Wait on semaphore. Updated count: %d\n", sem->count);
	if (sem->count < 0) {

		current_running_thread->state = BLOCKED;
		enqueueInQueue(sem->blocking_queue, (void *)current_running_thread);

		sem->threads_waiting = sem->threads_waiting + 1;
		printf("Blocked: %u\nThreads waiting: %d\n",
				current_running_thread->tid,
				sem->threads_waiting);

	}
	ENABLE_INTERRUPTS();
	// Wait for interrupt if blocked
	int i = 0;
	while (current_running_thread->state == BLOCKED)
		for (i = 0 ; i < SEM_MAX; i++);
}

// It deletes the memory space of a semaphore.
void mysem_delete( my_sem_t* sem ) {
	free(sem);
}
