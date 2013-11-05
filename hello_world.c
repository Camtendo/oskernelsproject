/*
 * Project 2
 * Cameron Crockrom
 * Sarah Fanning
 *
 * This program runs a continuous method "prototype_os" to simulate the operating system.
 * An alarm interrupts the "prototype_os" method.
 * These interrupts are then handled by our "myinterrupt_hander"
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

Node *(heads[5]);
Node *(running_thread[2]);

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
        	//new_node->previous = heads[status]->previous;
        	heads[status]->previous = new_node;
        	new_node->next = heads[status];
//                new_node->next = heads[status]; // wrap around
//                new_node->previous = heads[status]->previous;
//                new_node->previous->next = new_node;
//                heads[status]->previous = new_node; //wrap around
        }
}
Node * pop(int status) {
	Node *popped = NULL;
	if (heads[status] == NULL) //0 nodes
	{
		//alt_printf("Can't pop");
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
//		popped = heads[status];
//		heads[status] = popped->next; // update head
//		heads[status]->previous = popped->previous; // prev
//		heads[status]->previous->next = heads[status]; // prev
		return popped;
	}
	return popped;
}

//FIXME: not sure if correct
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

//FIXME: not checked
Node * lookup_node(int id, int status) {
        Node * node = heads[status];
        if (node->thread.thread_id == id)
                                return node;
        node = node->next;
        while (node != heads[status])
        {
                if (node->thread.thread_id == id)
                        return node;
                node = node->next;
        }
        return 0;
}

void prototype_os();
unsigned long long mythread_scheduler(unsigned long long param_list);
alt_u32 mythread_handler(void *param_list);
void mythread(int thread_id);
void mythread_create(TCB *tcb, void *(*start_routine)(void*), int thread_id);
void mythread_join(TCB *tcb);
void mythread_cleanup();
int timer_interrupt_flag;

void prototype_os()
{
        int i = 0;
        running_thread[1] = NULL;
        TCB *threads[NUM_THREADS];
        for (i = 0; i < NUM_THREADS; i++)
        {
                // Here: call mythread_create so that the TCB for each thread is created
                TCB *tcb = (TCB *) malloc(sizeof(TCB));
                mythread_create(tcb, &mythread, i);
                threads[i] = tcb;
        }
        // Here: initialize the timer and its interrupt handler as is done in Project I
        alt_alarm * myAlarm;
        alt_alarm_start( &myAlarm, ALARMTICKS(QUANTUM_LENGTH), &mythread_handler, NULL);
        for (i = 0; i < NUM_THREADS; i++)
        {
                // Here: call mythread_join to suspend prototype_os
                mythread_join((threads[i]));
        }

        while (TRUE)
        {
                alt_printf ("This is the OS prototype for my exciting CSE351 course projects!\n");
                int j = 0;
                for (j = 0 ; j < MAX; j++);
        }
}

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
        //alt_printf("Thread: %x Status:%x:\n",next->thread->thread_id, next->thread->scheduling_status);
        if (next != NULL  && next->thread.scheduling_status == READY)
        {
                // The context of the second thread (1) is crap. Something is probably wrong with creation or join. Else there's a problem in assembly with storing the fp
                if (running_thread[1]->thread.scheduling_status == RUNNING) {
                	running_thread[1]->thread.scheduling_status = READY;
                    add_node(running_thread[1], READY);

                }
                running_thread[1] = (Node *) malloc(sizeof(Node));
                running_thread[1]->thread = next->thread;
                //running_thread[1]->thread->thread_id = next->thread->thread;
                running_thread[1]->thread.scheduling_status = RUNNING;
        }
        else // No other threads available
        {
                alt_printf("Interrupted by the DE2 timer!\n");
                return 0;
        }
        int vals[2];
        vals[0]=running_thread[1]->thread.sp;
        vals[1]=running_thread[1]->thread.fp;
        // Return the new context
        return *vals;
}

alt_u32 mythread_handler(void *param_list)
{
        // Here: the global flag is used to indicate a timer interrupt
        timer_interrupt_flag = 1;
        alt_printf("Interrupted by the timer!\n");
        return ALARMTICKS(QUANTUM_LENGTH);
}

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

void mythread_create(TCB *tcb, void *(*start_routine)(void*), int thread_id)
{
	alt_printf("Creating...\n");
	        // Creates a Thread Control Block for a thread
	        tcb->thread_id = thread_id;
	        tcb->scheduling_status = READY;
	        tcb->context = malloc(4000);
	        tcb->fp = tcb->context + 4000/4;
	        tcb->sp = tcb->context + 128/4;
	        //tcb->context = malloc(768) + 128;
	        int one = 1;
	        void *(*ra)(void *) = &mythread_cleanup;
	        memcpy(tcb->sp + 0, &ra, 4);//ra
	        memcpy(tcb->sp + 20/4, &thread_id, 4);//r4?
	        memcpy(tcb->sp + 72/4, &start_routine, 4);//ea
	        memcpy(tcb->sp + 68/4, &one, 4);//estatus
	        memcpy(tcb->sp + 84/4, &tcb->fp, 4);//fp should be at 2048 if calloc returns addr 1024?

        // Add to ready queue
        Node *node = (Node *) malloc(sizeof(Node));
        node->thread = *tcb;
        add_node(node, READY);
        alt_printf("Finished creation (%x): sp: (%x)\n", thread_id, tcb->context);
}

void mythread_join(TCB *tcb) //WTF this method only gets called once, so it must naturally suspend the calling thread, preventing the other 7 calls Camtendo 11/4
{
	if (tcb->scheduling_status != DONE){
		while (running_thread[1] == NULL)
		{
			int i;
			for (i = 0 ; i < MAX; i++);
		}
		//DISABLE_INTERRUPTS();
		alt_printf("Joining.\n");
		int id = running_thread[1]->thread.thread_id;
		add_node(running_thread[1], WAITING);
		if (tcb->scheduling_status != DONE){
			tcb->blocking_id = id;
			running_thread[1]->thread.scheduling_status = WAITING;
		}
		while(running_thread[1]->thread.scheduling_status == WAITING)
		{
			int i;
			for (i = 0 ; i < MAX; i++);
		}
		//ENABLE_INTERRUPTS();
	}

}

// Set return address to this
void mythread_cleanup()//mythread_done
{
        //DISABLE_INTERRUPTS(); //Disabling interrupts here prevents the thread from completing for some reason... Camtendo 11/4
        alt_printf("COMPLETING THREAD\n");


        // Unblock thread blocked by join
        Node * blocked_node = lookup_node(running_thread[1]->thread.blocking_id, WAITING); //Blocking ID was not the expected value Camtendo 11/4
        if (blocked_node != NULL)
        {
                blocked_node->thread.scheduling_status = READY;
                add_node(blocked_node, READY);
        }
        add_node(running_thread[1], DONE);
        running_thread[1]->thread.scheduling_status = DONE;
        //ENABLE_INTERRUPTS();
        alt_printf("COMPLETED\n");
        while(TRUE);
}

int check_timer_flag()
{
        return timer_interrupt_flag; //returns in registers (2 and 3?)
}

void reset_timer_flag()
{
        timer_interrupt_flag = 0; //returns in registers (2 and 3?)
}

// The main method that starts up the prototype operating system
int main()
{
        alt_printf("Hello from Nios II!\n");
        prototype_os();
        return 0;
}
