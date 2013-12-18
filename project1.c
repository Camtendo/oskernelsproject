#include <stdio.h>

#include "alarm_handler.h"
#include "thread_handler.h"
#include "semaphore.h"

#define NUM_THREADS 8

/* a delay time used to adjust the frequency of printf messages */
#define MAX 70000
#define BUFFER_SIZE 6
char circularBuffer[BUFFER_SIZE];
static int nextBufferIndex = 0;
static int currentBufferSize = 0;
static my_sem_t *full = NULL;
static my_sem_t *empty = NULL;
static my_sem_t *mutex = NULL;


void mythread(unsigned int tid)
{
    unsigned int i, j, n;
    
    n = (tid % 2 == 0)? 10: 15;
    for(i = 0; i < n; i++)
    {
        printf("This is message %d of thread #%d.\n", i, tid);
        for(j = 0; j < (tid%3+1)*MAX; j++);
        // for (j = 0; j < MAX; j++);
    }   
}
void addX () {
	circularBuffer[nextBufferIndex] = 'X';
	nextBufferIndex = (nextBufferIndex + 1) % BUFFER_SIZE;
	currentBufferSize += 1;
	printBuffer(circularBuffer);
}

void removeX () {
	int remIndex = (nextBufferIndex - currentBufferSize + BUFFER_SIZE) % 6;
	circularBuffer[remIndex] = 'O';
	currentBufferSize -= 1;
	printBuffer(circularBuffer);
}
// Provided thread code
void producer(unsigned int thread_id)
{
	// The declaration of j as an integer was added on 10/24/2011
	// The threads with ID#(0 - 3) perform insertion/deletion operations for 10 times
	// while other threads perform the operations for 15 times
	int i, j, n;
	n = (thread_id < 4)? 10: 15;
	for (i = 0; i < n; i++)
	{
		printf("This is action %d of producer #%d.\n", i, thread_id);
		// Wait on empty
		printf("EMPTY\n");
		mysem_wait(empty);
		// Wait on mutex
		printf("MUTEX\n");
		mysem_wait(mutex);
		// modify the buffer
		addX();
		// Release stuff, up full
		printf("MUTEX\n");
		mysem_signal(mutex);
		printf("FULL\n");
		mysem_signal(full);
		for (j = 0; j < MAX; j++);
	}
}

// Provided thread code
void consumer(unsigned int thread_id)
{
	// The declaration of j as an integer was added on 10/24/2011
	int i, j, n;
	n = (thread_id < 4)? 10: 15;
	for (i = 0; i < n; i++)
	{
		printf("This is action %d of consumer #%d.\n", i, thread_id);
		// Wait on full
		printf("FULL\n");
		mysem_wait(full);
		// Wait on mutex
		printf("MUTEX\n");
		mysem_wait(mutex);
		// modify the buffer
		removeX();
		// Release mutex, up empty
		printf("MUTEX\n");
		mysem_signal(mutex);
		printf("EMPTY\n");
		mysem_signal(empty);
		for (j = 0; j < MAX; j++);
	}
}

void printBuffer(char buffer[]){
	int i;
	for (i = 0; i < BUFFER_SIZE; i++)
		printf("%c", buffer[i]);
	printf("\n");
}



    
void os_primitive()
{
	nextBufferIndex = 0;
	full = (my_sem_t*) malloc(sizeof(my_sem_t));
	empty = (my_sem_t*) malloc(sizeof(my_sem_t));
	mutex = (my_sem_t*) malloc(sizeof(my_sem_t));

	mysem_create(full, 0);
	mysem_create(empty, BUFFER_SIZE);
	mysem_create(mutex, 1);
	unsigned int i;
	tcb *thread_pointer;

	for (i = 0; i < NUM_THREADS; i++)
	{
    	if (i % 2 == 0)
    		thread_pointer = mythread_create(i, 4096, consumer);
    	else
    		thread_pointer = mythread_create(i, 4096, producer);
    	printf("Created thread with id: %u\n", thread_pointer->tid);
    	//thread_pointer = mythread_create(i, 4096, mythread);   // 4B * 4096 entries = 16KB
    	mythread_start(thread_pointer);
    	mythread_join(thread_pointer);
    }
    
    if ( start_alarm_succeed() )
        printf ("Start the alarm successfully\n");
    else
        printf ("Unable to start the alarm\n");

    int full_deleted = 0;
    int empty_deleted = 0;
    int mutex_deleted = 0;
    /* an endless while loop */
    while (1)
    {
        printf ("This is the OS primitive for my exciting CSE351 course projects!\n");
        
        /* delay printf for a while */
        for (i = 0; i < 10*MAX; i++);
        if (!full_deleted) mysem_delete(full);
        if (!empty_deleted) mysem_delete(empty);
        if (!mutex_deleted) mysem_delete(mutex);
    }
}

int main()
{
    os_primitive();
    return 0;
}
