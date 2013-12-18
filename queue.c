#include <stdio.h>
#include "queue.h"

static Q_type static_queue = {NULL, NULL, 0};

void enqueue(void *data)
{
	enqueueInQueue(&static_queue, data);
}

void enqueueInQueue(Q_type *_queue, void *data)
{
    E_type  *elem;
    
    if ((elem = (E_type *)malloc(sizeof(E_type))) == NULL)
    {
        printf("Unable to allocate space!\n");
        exit(1);
    }
    elem->data = data;
    elem->next = NULL;
    
    if (_queue->head == NULL)
    	_queue->head = elem;
    else
    	_queue->tail->next = elem;
    _queue->tail = elem;

    _queue->size++;
}
void *dequeue()
{
	return dequeueFromQueue(&static_queue);
}
void *dequeueFromQueue(Q_type *_queue)
{
    E_type  *elem;
    void    *data = NULL;
    
    if (_queue->size != 0)
    {
        elem = _queue->head;
        if (_queue->size == 1)
        	_queue->tail = NULL;
        _queue->head = _queue->head->next;
        
        _queue->size--;
        data = elem->data;
        free(elem);
    }
        
    return data;
}
unsigned int getQsize()
{
	return getQsizeOfQueue(&static_queue);
}
unsigned int getQsizeOfQueue(Q_type *_queue)
{
    return _queue->size;
}
