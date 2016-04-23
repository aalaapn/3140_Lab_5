/*Origninal code by 3140 Staff*/
#include "3140_concur.h"
#include <stdlib.h>

/* Global process pointers */
process_t* current_process = NULL;	/* Currently-running process */
process_t* process_queue = NULL;	/* Points to head of process queue */
/*Global time variable*/
realtime_t* current_time;
int process_deadline_miss;

/*------------------------------------------------------------------------
 * process_state
 *   Bookkeeping structure, holds relevant information about process.
 *   (Declared and typedef'd to process_t in 3140_concur.h)
 *   Fields:
 *     sp   - current stack pointer for process
 *     next - In this implementation, process_state struct is also a
 *            linked-list node (for use in a process queue)
 *----------------------------------------------------------------------*/

struct process_state {
	unsigned int sp;				/* Stack pointer for process */
	struct process_state* next;		/* Pointer to next process in queue */
	realtime_t* arrival_time;
	realtime_t* deadline;
};

/*-----------------------------------------------------------------------
 *implementation of current time
 *-----------------------------------------------------------------------*/



/*------------------------------------------------------------------------
 * Process queue management convenience functions
 *----------------------------------------------------------------------*/

/* Add process p to the tail of process queue */
void add_to_tail(process_t** head_ref, process_t* p, realtime_t *deadline) {
	/* Get pointer to the current head node */
	process_t* current = *head_ref;
	process_t* temp;
	
	/* If queue is currently empty, replace it with the new process */
	if (current == NULL) { *head_ref = p; }
	
	/* Otherwise, find the end of the list and append the new node */
	else {
		if (deadline ==NULL){
			while (current->next != NULL) { current = current->next; }
			/* At this point, current points to the last node in the list */
			current->next = p;
		}
		else{
			while(current->next !=NULL && current->deadline < current->next->deadline){
				current = current->next;
				
			}
			temp = current->next;
			current->next = p;
			p->next = temp;
		}
	}
}

/* Remove and return (pop) process from head of process queue */
process_t* take_from_head(process_t** head_ref) {
	/* We want to return the current head process */
	process_t* result = *head_ref;
	
	/* Remove the first process, unless the queue is empty */
	if (result != NULL) {
		*head_ref = result->next;	/* New head is the next process in queue */
		result->next = NULL;		/* Removed process no longer points to queue */
	}
	
	return result;
}


/*------------------------------------------------------------------------
 *  process_create
 *    Allocate stack space for process, initialize bookkeeping structures
 *    Returns 0 if process is created successfully, -1 otherwise.
 * 
 *    f: pointer to function where the process should begin execution
 *    n: initial process stack size (in words)
 *----------------------------------------------------------------------*/


int process_create(void (*f)(void), int n) {
	/* Allocate bookkeeping structure for process */
	//process_t* new_proc;
	process_t* new_proc = (process_t*) malloc(sizeof(process_t));
  if (new_proc == NULL) { return -1; }	/* malloc failed */
	
	/* Allocate and initialize stack space for process */
	//new_proc = (process_t*) our_malloc(sizeof(process_t)/4);
	new_proc->sp = process_init(f, n);
	
	if (new_proc->sp == 0) { return -1; }	/* process_init failed */
	
	/* Add new process to process queue */
	new_proc->next = NULL;
	add_to_tail(&process_queue, new_proc,new_proc->deadline);
	return 0;	/* Successfully created process and bookkeeping */
}
 
 
 /*------------------------------------------------------------------------
  *  process_start
  *    Launch concurrent execution of processes (must be created first).
  *----------------------------------------------------------------------*/
  
void process_start(void) {\
	/*Piazza code*/
	NVIC_SetPriority(SVCall_IRQn, 1);
	NVIC_SetPriority(PIT0_IRQn, 1);
	NVIC_SetPriority(PIT1_IRQn, 0);
	
	PIT->CHANNEL[0].TCTRL = 1; // Disable PIT0
	__enable_irq(); // Enable global interrupts
	// your busy-wait code here
	__disable_irq(); // Disable global interrupts
	PIT->CHANNEL[0].TCTRL = 3; // Enable PIT0


	
	/* Set up Timer A (triggers context switch) */
	SIM->SCGC6 |= SIM_SCGC6_PIT_MASK; // CLOCK PIT
	PIT->MCR = 0x0;	// turn on PIT
	PIT->CHANNEL[0].LDVAL = 0x0100000;
	PIT->CHANNEL[0].TCTRL  = 3; //|= (1 << 28) | (1<<29) | (1<<30);	
	
	/*Set up Timer B (tracks real time elapsed*/
	//Use a PIT timer, every milisecond it generates an interrupt 
	
	PIT->CHANNEL[1].LDVAL = 0x20900; //one milisecond
	PIT->CHANNEL[1].TCTRL = 1; //enable timer

	NVIC_EnableIRQ(PIT0_IRQn); //Enable interrupts!!!!!!!!!!!
	process_begin();	/* In assembly, actually launches processes */
}

void PIT1_IRQHandler(void)
{
	PIT->CHANNEL[1].TCTRL &= ~PIT_TCTRL_TEN_MASK; //disabling the timer so that a new value can be loaded
	
	if(current_time->msec<1000){
	current_time->msec+=1;
	}else{
	current_time->sec+=1;
	current_time->msec=0;
	}
	PIT->CHANNEL[1].TFLG = 1; //Clear interrupts
	PIT->CHANNEL[1].LDVAL = 20900; //reload value
	PIT->CHANNEL[1].TCTRL |= PIT_TCTRL_TEN_MASK;//enable the timer so that new timer can count down
	PIT_TCTRL1 |= PIT_TCTRL_TIE_MASK;//enable timer interrupts  
}

 /*------------------------------------------------------------------------
  *  process_select
  *    Returns the stack pointer for the next process to execute, or 0
  *    if there are no more processes to run.
  * 
  *    cursp: stack pointer of currently running process, or 0 if there
  *           is no process currently running
  *----------------------------------------------------------------------*/

unsigned int process_select(unsigned int cursp) {
	/* cursp==0 -> No process currently running */
	if (cursp == 0) {
		current_process = NULL;
		if (process_queue == NULL) { 
		  return 0; 
		}	/* No processes left */
		else {
			/* Return next process from queue */
			current_process = take_from_head(&process_queue);
			return current_process->sp;
		}
	}
	
	/* cursp != 0 -> Some running process was interrupted */
	else {
		/* Save running process SP and add back to process queue to run later */
		current_process->sp = cursp;
		add_to_tail(&process_queue, current_process, current_process->deadline);
		
		/* Return next process from queue */
		current_process = take_from_head(&process_queue);
		return current_process->sp;
	}
}
/*-----------------------------------------
 *Function for Lab 5
 *-----------------------------------------*/
int procces_rt_create(void (*f)(void), int n, realtime_t *start, realtime_t *work, realtime_t *deadline){
	//task requres "work" miliseconds to complete (estimate of worst case execution time)
	//relative deadline of "deadline" miliseconds
	//n is the stack size for the task.
	
	//stuff below taken from process_create
	process_t* new_proc = (process_t*) malloc(sizeof(process_t));
	if (new_proc == NULL) { return -1; }	/* malloc failed */
	
	/* Allocate and initialize stack space for process */
	//new_proc = (process_t*) our_malloc(sizeof(process_t)/4);
	new_proc->sp = process_init(f, n);
	
	if (new_proc->sp == 0) { return -1; }	/* process_init failed */
	
	/* Add new process to process queue */
	new_proc->next = NULL;
	add_to_tail(&process_queue, new_proc, new_proc->deadline);
	return 0;	/* Successfully created process and bookkeeping */
	//end stuff taken from process_create
	
	
	
}
