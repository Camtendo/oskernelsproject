/* 
    Name:   queue.h
    Author: Dongyuan Zhan
    Date:   11/20/2010

Description:
    Functions used to implment a queue and its operations
*/

typedef struct element_type
{
    void                *data;
    struct element_type *next;
} E_type;

typedef struct queue_type
{
    E_type              *head;
    E_type              *tail;
    unsigned int        size;
} Q_type;

void enqueue(void *data);

void *dequeue();

unsigned int getQsize();

void enqueueInQueue(Q_type *queue, void *data);

void *dequeueFromQueue(Q_type *queue);

unsigned int getQsizeOfQueue(Q_type *queue);
