#include <stdio.h>
#include <stdlib.h>
#include "queue.h"
#include <string.h>

int empty(struct queue_t * q) {
	return (q->size == 0);
}



void enqueue(struct queue_t * q, struct pcb_t * proc) {
	/* TODO: put a new process to queue [q] */	
	if (q->size < MAX_QUEUE_SIZE)
	{
		q->size ++;
		q->proc[q->size-1] = proc;
	}
}


struct pcb_t * dequeue(struct queue_t * q) {
	/* TODO: return a pcb whose prioprity is the highest
	 * in the queue [q] and remember to remove it from q
	 * */
	if (!empty(q))
	{
		int index;
		int max_pri = 0;
		for (int i = 0; i<q->size ; i++)
		if (q->proc[i]->priority > max_pri)
		{
			index = i;
			max_pri = q->proc[i]->priority;
		}

		struct pcb_t *temp = q->proc[index];
		q->proc[index] = q->proc[q->size-1];
		q->proc[q->size - 1] = NULL;
		q->size--;
		return temp;
	}
	return NULL;
}

