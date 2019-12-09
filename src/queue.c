#include <stdio.h>
#include <stdlib.h>
#include "queue.h"
#include <string.h>

int empty(struct queue_t * q) {
	return (q->size == 0);
}


struct pcb_t * deep_copy(struct pcb_t * proc)
{
	struct pcb_t *temp = (struct pcb_t*)malloc(sizeof(struct pcb_t));
	temp->pid = proc-> pid;
	temp->pc = proc->pc;
	temp->priority = proc->priority;
	temp->bp = proc->bp;
	for (int i =0; i<10; i++) temp->regs[i] = proc->regs[i];
	
	if (proc->code != NULL)
	{
		temp->code = malloc(sizeof(struct code_seg_t));
		temp->code->size = proc->code->size;
		temp->code->text = malloc(sizeof(struct inst_t));
		memmove(&temp->code->text , &proc->code->text , sizeof(struct inst_t));
		//free(proc->code);
	}
	
	if (proc->seg_table != NULL)
	{
		temp->seg_table = malloc(sizeof(struct seg_table_t));
		temp->seg_table->size = proc->seg_table->size;
		for (int i =0; i<32 ; i++)
		{
			temp->seg_table->table[i].v_index = proc->seg_table->table[i].v_index;
			temp->seg_table->table[i].pages = malloc(sizeof(struct page_table_t));
			memmove(&temp->seg_table->table[i].pages , &proc->seg_table->table[i].pages , sizeof(struct page_table_t));
			//free(proc->seg_table->table[i].pages);
		}
		//free(proc->seg_table);
	}
	free(proc);
	return temp;
}
void enqueue(struct queue_t * q, struct pcb_t * proc) {
	/* TODO: put a new process to queue [q] */	
	if (q->size <MAX_QUEUE_SIZE)
	{
		for (int i =0; i<MAX_QUEUE_SIZE ; i++)
		if (q->proc[i] ==NULL)
		{
			q->proc[i] = proc;
			q->size ++;
			break;
		}
	}
}


struct pcb_t * dequeue(struct queue_t * q) {
	/* TODO: return a pcb whose prioprity is the highest
	 * in the queue [q] and remember to remove it from q
	 * */
	if (!empty(q))
	{
		int index = -1;
		int max_pri = 0;
		for (int i =0; i<MAX_QUEUE_SIZE ; i++)
		if (q->proc[i] !=NULL && q->proc[i]->priority> max_pri)
		{
			index = i;
			max_pri = q->proc[i]->priority;
		}

		//struct pcb_t *temp = deep_copy(q->proc[index]);
		struct pcb_t *temp = q->proc[index];
		q->proc[index] = NULL;
		q->size--;
		return temp;
		
	}
	return NULL;
}

