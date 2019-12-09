
#ifndef QUEUE_H
#define QUEUE_H

#include "common.h"
#include <stdio.h>

#define MAX_QUEUE_SIZE 10

struct queue_t {
	struct pcb_t * proc[MAX_QUEUE_SIZE];
	int size;
};

struct pcb_t * deep_copy(struct pcb_t * proc);

void enqueue(struct queue_t * q, struct pcb_t * proc);

struct pcb_t * dequeue(struct queue_t * q);

int empty(struct queue_t * q);

#endif

