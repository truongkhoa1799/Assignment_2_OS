
#include "cpu.h"
#include "timer.h"
#include "sched.h"
#include "loader.h"
#include "mem.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int time_slot;
static int num_cpus;
static int done = 0;

static struct ld_args{
	char ** path;
	unsigned long * start_time;
} ld_processes;

int num_processes;

struct cpu_args {
	struct timer_id_t * timer_id;
	int id;
};

static void * cpu_routine(void * args) {
	struct timer_id_t * timer_id = ((struct cpu_args*)args)->timer_id;
	int id = ((struct cpu_args*)args)->id;
	/* Check for new process in ready queue */
	int time_left = 0;
	struct pcb_t * proc = NULL;

	while (1) {
		/* Check the status of current process */
		if (proc == NULL) {
			/* No process is running, the we load new process from
		 	* ready queue */
			proc = get_proc();
		}else if (proc->pc == proc->code->size) {
			/* The porcess has finish it job */
			printf("\tCPU %d: Processed %2d has finished\n", id ,proc->pid);
			free(proc);
			proc = get_proc();
			time_left = 0;
		}else if (time_left == 0) {
			/* The process has done its job in current time slot */
			printf("\tCPU %d: Put process %2d to run queue\n", id, proc->pid);
			put_proc(proc);
			proc = get_proc();
			/*WE RESET THE TIME_LEFT WHEN WE LOAD THE NEW PROCESS*/
		}
		
		/* Recheck process status after loading new process */
		if (proc == NULL && done) {
			/* we check done because may be it is not a time to load the process
			 *And done = 1 if and only if the load routine is done.
			 * No process to run, exit */
			printf("\tCPU %d stopped\n", id);
			break;
		}else if (proc == NULL) {
			/* There may be new processes to run in
			 * next time slots, just skip current slot 
			 * there is no process in the ready queue. because it
			 * is not loaded into the ready queue*/
			next_slot(timer_id);
			continue;
		}else if (time_left == 0) {
			/*the period for each process is over. then we load the next process*/
			printf("\tCPU %d: Dispatched process %2d\n", id, proc->pid);
			time_left = time_slot;
		}
		
		/* Run current process */
		run(proc);
		time_left--;
		next_slot(timer_id);
	}
	detach_event(timer_id);
	pthread_exit(NULL);
}

static void * ld_routine(void * args) {
	struct timer_id_t * timer_id = (struct timer_id_t*)args;
	int i = 0;
	while (i < num_processes) {
		struct pcb_t * proc = load(ld_processes.path[i]);
		while (current_time() < ld_processes.start_time[i]) 
		{
			next_slot(timer_id);
		}
		printf("\tLoaded a process at %s, PID: %d\n", ld_processes.path[i], proc->pid);
		add_proc(proc);
		free(ld_processes.path[i]);
		i++;
		next_slot(timer_id);
	}
	free(ld_processes.path);
	free(ld_processes.start_time);
	done = 1;
	detach_event(timer_id);
	pthread_exit(NULL);
}

static void read_config(const char * path) {
	FILE * file;
	if ((file = fopen(path, "r")) == NULL) {
		printf("Cannot find configure file at %s\n", path);
		exit(1);
	}
	fscanf(file, "%d %d %d\n", &time_slot, &num_cpus, &num_processes);
	ld_processes.path = (char**)malloc(sizeof(char*) * num_processes);
	ld_processes.start_time = (unsigned long*) malloc(sizeof(unsigned long) * num_processes);
	int i;

	for (i = 0; i < num_processes; i++) {
		ld_processes.path[i] = (char*)malloc(sizeof(char) * 100);
		ld_processes.path[i][0] = '\0';
		strcat(ld_processes.path[i], "input/proc/");
		char proc[100];
		fscanf(file, "%lu %s\n", &ld_processes.start_time[i], proc);
		strcat(ld_processes.path[i], proc);
	}
}

int main(int argc, char * argv[]) {
	/* Read config */
	if (argc != 2) {
		printf("Usage: os [path to configure file]\n");
		return 1;
	}
	char path[100];
	path[0] = '\0';
	strcat(path, "input/");
	strcat(path, argv[1]);
	read_config(path);

	// create thread for cpus
	pthread_t * cpu = (pthread_t*)malloc(num_cpus * sizeof(pthread_t));
	
	//create arguememnts for cpus
	struct cpu_args * args = (struct cpu_args*)malloc(sizeof(struct cpu_args) * num_cpus);
	
	// create thread for loader
	pthread_t ld;
	
	/* Init timer with n cpus thread and load thread*/
	int i;
	for (i = 0; i < num_cpus; i++) {
		// assigned each cpus for event of timer.
		args[i].timer_id = attach_event();
		args[i].id = i;
	}
	// assign event loader for timer.
	struct timer_id_t * ld_event = attach_event();
	start_timer();

	/* Init scheduler */
	init_scheduler();

	/* Run CPU and loader. Assigned the thread to cpus and loader*/
	pthread_create(&ld, NULL, ld_routine, (void*)ld_event);
	/**For each cpus we assign it into one thread. with the same code text.
	 * Therefore, they can run concuruncy*/
	for (i = 0; i < num_cpus; i++) {
		pthread_create(&cpu[i], NULL , cpu_routine, (void*)&args[i]);
	}

	/* Wait for CPU and loader finishing */
	for (i = 0; i < num_cpus; i++) {
		pthread_join(cpu[i], NULL);
	}
	pthread_join(ld, NULL);

	/* Stop timer */
	stop_timer();

	printf("\nMEMORY CONTENT: \n");
	dump();

	return 0;

}



