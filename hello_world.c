/*
 * Project 2
 * Cameron Crockrom
 * Sarah Fanning
 *
 * This program runs a continuous method "prototype_os" to simulate the operating system.
 * An alarm interrupts the "prototype_os" method.
 * These interrupts cause the scheduler to run.
 * Output is provided to stdout to indicate what the program is doing.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "sys/alt_alarm.h"
#include "sys/stdio.h"
#include "alt_types.h"
#define TRUE 1
#define FALSE 0
#define ALARMTICKS(x) ((alt_ticks_per_second()*(x))/10)
#define MAX 50000
#define QUANTUM_LENGTH 10
#define NUM_THREADS 8
#define RUNNING 0
#define READY 1
#define WAITING 2
#define START 3
#define DONE 4
#define BUFFER_SIZE 6
// disable an interrupt
#define DISABLE_INTERRUPTS() asm("wrctl status, zero");
// enable an interrupt
#define ENABLE_INTERRUPTS() asm("movi et, 1"); asm("wrctl status, et");

typedef struct {
	int thread_id;
	// (running, ready, waiting, start, done)
	int scheduling_status;
	int *context;
	int *sp;
	int *fp;
	int blocking_id;
} TCB;

typedef struct Node{
	TCB thread;
	struct Node *previous;
	struct Node *next;
} Node;

Node *(heads[8]);
int headCount = 5;
Node *(running_thread[2]);

typedef struct my_sem_t {
	int sem_blocking_id;
	int count;
	int threads_waiting;
} my_sem_t;



// Add a node to the specified status queue
void add_node(Node *new_node, int status) {
	if (heads[status] == NULL) // 0 nodes
	{
		heads[status] = new_node;
		heads[status]->next = NULL;
		heads[status]->previous = NULL;
	}
	else if (heads[status]->next == NULL) // 1 node
	{
		heads[status]->next = new_node;
		heads[status]->previous = new_node;
		new_node->next = heads[status];
		new_node->previous = heads[status];
	}
	else // 2+ nodes
	{
		new_node->previous = heads[status]->previous;
		new_node->previous->next = new_node;
		heads[status]->previous = new_node;
		new_node->next = heads[status];
	}
}

// Pop the first node from the specified status queue
Node * pop(int status) {
	Node *popped = NULL;
	if (heads[status] == NULL) //0 nodes
	{
		// Do nothing. Popped is already NULL
	}
	else if (heads[status]->next == NULL) //1
	{
		popped = heads[status];
		heads[status] = NULL;
		return popped;
	}
	else if (heads[status]->next == heads[status]->previous) //2
	{
		popped = heads[status];
		heads[status] = popped->next;
		heads[status]->next = NULL;
		heads[status]->previous = NULL;
	}
	else //3+
	{
		popped = heads[status];
		heads[status] = popped->next;
		Node *last = popped->previous;
		popped->next->previous = last;
		last->next = popped->next;
		popped->next = NULL;
		popped->previous = NULL;
		return popped;
	}
	return popped;
}

// Remove a node from the specified status queue
void remove_node(Node *node, int status) {
	if (node->next == NULL) //1 node
	{
		if (node == heads[status])
		{
			heads[status] = NULL;
		}
	}
	else if (heads[status]->next == heads[status]->previous)//2
	{
		heads[status] = node->next;
		heads[status]->next = NULL;
		heads[status]->previous = NULL;
	}
	else//2+
	{
		Node *previous = node->previous;
		Node *next = node->next;
		previous->next = next;
		next->previous = previous;
		if (node == heads[status])
		{
			heads[status] = next;
		}
	}
}

// Lookup a node in the specified status queue
Node * lookup_node(int id, int status) {
	Node * node = heads[status];
	if (node == NULL) {
		return -1;
	}
	else if (node->next == NULL)
	{
		if (node->thread.thread_id == id)
			return node;
		else
			return -1;
	}
	else
	{
		if (node->thread.thread_id == id)
			return node;
		node = node->next;
		while (node != heads[status])
		{
			if (node->thread.thread_id == id)
				return node;
			node = node->next;
		}
	}
	return -1;
}

void prototype_os();
unsigned long long mythread_scheduler(unsigned long long param_list);
alt_u32 mythread_handler(void *param_list);
void mythread(int thread_id);
void mythread_create(TCB *tcb, void *(*start_routine)(void*), int thread_id);
void mythread_join(int thread_id);
void mythread_cleanup();
my_sem_t* mysem_create( int initialCount );
void mysem_signal( my_sem_t* sem );
void mysem_wait( my_sem_t* sem );
void mysem_delete( my_sem_t* sem );
int timer_interrupt_flag;
char circularBuffer[BUFFER_SIZE];
int nextBufferIndex = 0;
void producer(int thread_id);
void consumer(int thread_id);

my_sem_t *full;
my_sem_t *empty;
my_sem_t *mutex;

// Our operating system prototype
void prototype_os()
{
	full = mysem_create(0);
	empty = mysem_create(BUFFER_SIZE);
	mutex = mysem_create(1);
	int i = 0;
	running_thread[1] = NULL;
	TCB *threads[NUM_THREADS];
	for (i = 0; i < NUM_THREADS; i++)
	{
		// Here: call mythread_create so that the TCB for each thread is created
		TCB *tcb = (TCB *) malloc(sizeof(TCB));
		if (i % 2 == 0)
			mythread_create(tcb, &consumer, i);
		else
			mythread_create(tcb, &producer, i);
		threads[i] = tcb;
	}
	// Here: initialize the timer and its interrupt handler as is done in Project I
	alt_alarm * myAlarm;
	alt_alarm_start( &myAlarm, ALARMTICKS(QUANTUM_LENGTH), &mythread_handler, NULL);
	for (i = 0; i < NUM_THREADS; i++)
	{
		// Here: call mythread_join to suspend prototype_os
		mythread_join(i);
	}

	while (TRUE)
	{
		alt_printf ("This is the OS prototype for my exciting CSE351 course projects!\n");
		int j = 0;
		for (j = 0 ; j < MAX * 10; j++);
	}
}



// This is the scheduler. It works with Injection.S to switch between threads
unsigned long long mythread_scheduler(unsigned long long param_list) // context pointer
{
	int * param_ptr = &param_list;
	// If running thread is null, then store the context and add to the run queue
	if (running_thread[1] == NULL)
	{
		// Store a new context (os_prototype, most likely)
		TCB *tcb = (TCB *) malloc(sizeof(TCB));
		tcb->thread_id = NUM_THREADS + 1; // TODO:set to something legitimate
		tcb->sp = *param_ptr; // context pointer <---not sure this is correct?
		tcb->fp = *(param_ptr+1);
		Node *node = (Node *) malloc(sizeof(Node));
		node->thread = *tcb;
		running_thread[1] = node;
		running_thread[1]->thread.scheduling_status = RUNNING;
	}
	running_thread[1]->thread.sp = *param_ptr; // update context pointer
	running_thread[1]->thread.fp = *(param_ptr+1);

	// Here: perform thread scheduling
	Node *next = pop(READY);
	if (next != NULL  && next->thread.scheduling_status == READY)
	{
		// The context of the second thread (1) is crap. Something is probably wrong with creation or join. Else there's a problem in assembly with storing the fp
		if (running_thread[1]->thread.scheduling_status == RUNNING) {
			running_thread[1]->thread.scheduling_status = READY;
		}
		add_node(running_thread[1], running_thread[1]->thread.scheduling_status);
		running_thread[1] = (Node *) malloc(sizeof(Node));
		running_thread[1]->thread = next->thread;
		//running_thread[1]->thread->thread_id = next->thread->thread;
		running_thread[1]->thread.scheduling_status = RUNNING;
	}
	else // No other threads available
	{
		alt_printf("Interrupted by the DE2 timer!\n");
		running_thread[1]->thread.scheduling_status = RUNNING;
		return 0;
	}
	// Prepare values to return
	unsigned long long ret_list;
	int * rets = &ret_list;
	*(rets) = running_thread[1]->thread.sp;
	*(rets + 1) = running_thread[1]->thread.fp;
	return ret_list;
}

// Sets the timer_interrupt_flag that is checked by Injection.S
alt_u32 mythread_handler(void *param_list)
{
	// Here: the global flag is used to indicate a timer interrupt
	timer_interrupt_flag = 1;
	alt_printf("Interrupted by the timer!\n");
	return ALARMTICKS(QUANTUM_LENGTH);
}

// Provided thread code
void mythread(int thread_id)
{
	// The declaration of j as an integer was added on 10/24/2011
	int i, j, n;
	n = (thread_id % 2 == 0)? 10: 15;
	for (i = 0; i < n; i++)
	{
		printf("This is message %d of thread #%d.\n", i, thread_id);
		for (j = 0; j < MAX; j++);
	}
}

// Provided thread code
void producer(int thread_id)
{
	// The declaration of j as an integer was added on 10/24/2011
	// The threads with ID#(0 - 3) perform insertion/deletion operations for 10 times
	// while other threads perform the operations for 15 times
	int i, j, n;
	n = (thread_id < 4)? 10: 15;
	for (i = 0; i < n; i++)
	{
		// Wait on empty
		mysem_wait(empty);
		// Wait on mutex
		mysem_wait(mutex);
		// modify the buffer
		addX();
		// Release stuff, up full
		mysem_signal(mutex);
		mysem_signal(full);
		for (j = 0; j < MAX; j++);
	}
}

// Provided thread code
void consumer(int thread_id)
{
	// The declaration of j as an integer was added on 10/24/2011
	int i, j, n;
	n = (thread_id < 4)? 10: 15;
	for (i = 0; i < n; i++)
	{
		// Wait on full
		mysem_wait(full);
		// Wait on mutext
		mysem_wait(mutex);
		// modify the buffer
		removeX();
		// Release mutex, up empty
		mysem_signal(mutex);
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

void addX () {
	circularBuffer[nextBufferIndex] = 'X';
	nextBufferIndex = (nextBufferIndex + 1) % BUFFER_SIZE;
	printBuffer(circularBuffer);
}

void removeX () {
	nextBufferIndex = nextBufferIndex - 1;
	nextBufferIndex = (nextBufferIndex < 0 )? BUFFER_SIZE - 1 : nextBufferIndex;
	circularBuffer[nextBufferIndex] = 'O';
	printBuffer(circularBuffer);
}

// Creates a thread and adds it to the ready queue
void mythread_create(TCB *tcb, void *(*start_routine)(void*), int thread_id)
{
	alt_printf("Creating...\n");
	// Creates a Thread Control Block for a thread
	tcb->thread_id = thread_id;
	tcb->blocking_id = -1;
	tcb->scheduling_status = READY;
	tcb->context = malloc(4000);
	tcb->fp = tcb->context + 4000/4;
	tcb->sp = tcb->context + 128/4;

	int one = 1;
	void *(*ra)(void *) = &mythread_cleanup;
	memcpy(tcb->sp + 0, &ra, 4);//ra
	memcpy(tcb->sp + 20/4, &thread_id, 4);//r4?
	memcpy(tcb->sp + 72/4, &start_routine, 4);//ea
	memcpy(tcb->sp + 68/4, &one, 4);//estatus
	memcpy(tcb->sp + 84/4, &tcb->fp, 4);//fp

	// Add to ready queue
	Node *node = (Node *) malloc(sizeof(Node));
	node->thread = *tcb;
	add_node(node, READY);
	alt_printf("Finished creation (%x): sp: (%x)\n", thread_id, tcb->context);
}

// Joins the thread with the calling thread
void mythread_join(int thread_id)
{
	// Wait for timer the first time
	int i;
	while (running_thread[1] == NULL)
		for (i = 0 ; i < MAX; i++);
	int joined = FALSE;
	Node *temp = -1;
	temp = lookup_node(thread_id, READY);
	TCB *tcb;
	int calling_id = running_thread[1]->thread.thread_id;
	alt_printf("Joining if not finished.\n");

	temp = lookup_node(thread_id, READY);
	if (temp != 0xffffffff)
		tcb = &temp->thread;
	if (temp != 0xffffffff && tcb->scheduling_status != DONE){
		// Join the thread
		tcb->blocking_id = calling_id;
		running_thread[1]->thread.scheduling_status = WAITING;
		joined = TRUE;
	}

	if (joined == TRUE)
		alt_printf("Joined (%x)\n", thread_id);
	// Wait for timer
	while (running_thread[1]->thread.scheduling_status == WAITING)
		for (i = 0 ; i < MAX; i++);
}

// Threads return here and space is freed
void mythread_cleanup()
{
	// Unblock thread blocked by join
	DISABLE_INTERRUPTS();
	int id = running_thread[1]->thread.blocking_id;
	if (id > 0) {
		Node * temp = 0xffffffff;
		temp = lookup_node(running_thread[1]->thread.blocking_id, WAITING); //Blocking ID was not the expected value Camtendo 11/4
		if (temp != 0xffffffff) // not found
		{
			Node * blocked_node = (Node *) malloc(sizeof(Node));
			blocked_node->thread = temp->thread;
			blocked_node->thread.scheduling_status = READY;
			remove_node(temp, WAITING);
			add_node(blocked_node, READY);
		}
	}
	ENABLE_INTERRUPTS();
	alt_printf("COMPLETED.\n");
	DISABLE_INTERRUPTS();
	free(running_thread[1]->thread.context);
	running_thread[1]->thread.scheduling_status = DONE;
	ENABLE_INTERRUPTS();
	while(TRUE);
}

int check_timer_flag()
{
	return timer_interrupt_flag; //returns in registers (2 and 3)
}

void reset_timer_flag()
{
	timer_interrupt_flag = 0; //returns in registers (2 and 3)
}

// The main method that starts up the prototype operating system
int main()
{
	alt_printf("Hello from Nios II!\n");
	prototype_os();
	return 0;
}

// It returns the starting address a semaphore variable.
// You can use malloc() to allocate memory space.
my_sem_t* mysem_create( int initialCount ) {
	my_sem_t* semaphore = malloc(sizeof(my_sem_t));
	semaphore->count = initialCount;
	semaphore->threads_waiting = 0;
	semaphore->sem_blocking_id = headCount++;
	printf("Semaphore created with initial count: %d\n", semaphore->count);
	return semaphore;
}

// It performs the semaphore’s signal operation.
void mysem_signal( my_sem_t* sem ) {
	sem->count += 1;
	if (sem->count <= 0) {//<= or >=
		Node *blocked_node = heads[sem->sem_blocking_id];
		blocked_node->thread.scheduling_status = READY;
		remove_node(blocked_node, sem->sem_blocking_id);
		add_node(blocked_node, READY);
		sem->threads_waiting--;
		printf("Unblocked: %d\nThreads waiting: %d\n",
				blocked_node->thread.thread_id,
				sem->threads_waiting);
	}
}

// It performs the semaphore’s wait operation.
void mysem_wait( my_sem_t* sem ) {
	sem->count--;
	if (sem->count < 0) {
		running_thread[1]->thread.scheduling_status = sem->sem_blocking_id;
		sem->threads_waiting++;
		printf("Blocked: %d\nThreads waiting: %d\n",
				running_thread[1]->thread.thread_id,
				sem->threads_waiting);
		// Block
		int i = 0;
		while (running_thread[1]->thread.scheduling_status == sem->sem_blocking_id)
				for (i = 0 ; i < MAX; i++);
	}
}

// It deletes the memory space of a semaphore.
void mysem_delete( my_sem_t* sem ) {
	free(sem);
}
