#include "3140_concur.h"
#include <stdlib.h>

/* Global process pointers */
process_t* current_process = NULL;	/* Currently-running process */
process_t* process_queue = NULL;	/* Points to head of process queue */

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
};


/*------------------------------------------------------------------------
 * Process queue management convenience functions
 *----------------------------------------------------------------------*/

/* Add process p to the tail of process queue */
void add_to_tail(process_t** head_ref, process_t* p) {
	/* Get pointer to the current head node */
	process_t* current = *head_ref;
	
	/* If queue is currently empty, replace it with the new process */
	if (current == NULL) { *head_ref = p; }
	
	/* Otherwise, find the end of the list and append the new node */
	else {
		while (current->next != NULL) { current = current->next; }
		/* At this point, current points to the last node in the list */
		current->next = p;
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
	add_to_tail(&process_queue, new_proc);
	return 0;	/* Successfully created process and bookkeeping */
}
 
 
 /*------------------------------------------------------------------------
  *  process_start
  *    Launch concurrent execution of processes (must be created first).
  *----------------------------------------------------------------------*/
  
void process_start(void) {
	/* Set up Timer A (triggers context switch) */
	SIM->SCGC6 |= SIM_SCGC6_PIT_MASK; // CLOCK PIT
	PIT->MCR = 0x0;	// turn on PIT
	PIT->CHANNEL[0].LDVAL = 0x0100000;
	PIT->CHANNEL[0].TCTRL  = 3; //|= (1 << 28) | (1<<29) | (1<<30);	

	NVIC_EnableIRQ(PIT0_IRQn); //Enable interrupts!!!!!!!!!!!
	process_begin();	/* In assembly, actually launches processes */
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
		add_to_tail(&process_queue, current_process);
		
		/* Return next process from queue */
		current_process = take_from_head(&process_queue);
		return current_process->sp;
	}
}

