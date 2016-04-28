/*Origninal code by 3140 Staff*/
#include "3140_concur.h"
#include <stdlib.h>

/* Global process pointers */
process_t* current_process = NULL;	/* Currently-running process */
process_t* process_queue = NULL;	/* Points to head of process queue */
process_t* process_queue_rt = NULL; /*Point to head of process queue for rt processes*/
process_t* process_waiting_rt = NULL; /*Point to head of the process queue waiting to start*/
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
			/*This is a non realtime process*/
			while (current->next != NULL) { current = current->next; }
			/* At this point, current points to the last node in the list */
			current->next = p;
		}
		else{
			if(current->next==NULL){
			current->next=NULL;
			}
			/*Realtime Process*/
			else if(current->next==NULL && ((current->deadline->msec+current->deadline->sec*1000)>(deadline->msec+deadline->sec*1000))){
			*head_ref=p;
			temp=current;
			current=p;
			current->next=temp;
			}
			else if(current->next==NULL && ((current->deadline->msec+current->deadline->sec*1000)<(deadline->msec+deadline->sec*1000))){
			current->next=p;
			p->next=NULL;
			}
			
			else if (current->next!=NULL){
				
				while(current->next !=NULL && ((current->deadline->msec+current->deadline->sec*1000) < (current->next->deadline->msec+current->next->deadline->sec*1000))){
				current = current->next;
				}
				temp = current->next;
				current->next = p;
				p->next = temp;
				
			}
			
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
	
	new_proc->deadline=NULL;
	new_proc->arrival_time=NULL;
	
	if (new_proc->sp == 0) { return -1; }	/* process_init failed */
	/* Add new process to process queue */
	new_proc->next = NULL;
	add_to_tail(&process_queue, new_proc, NULL);
	return 0;	/* Successfully created process and bookkeeping */
}
 
 
 /*------------------------------------------------------------------------
  *  process_start
  *    Launch concurrent execution of processes (must be created first).
  *----------------------------------------------------------------------*/
  
void process_start(void) {\
	/*Piazza code*/
	


	
	/* Set up Timer A (triggers context switch) */
	SIM->SCGC6 |= SIM_SCGC6_PIT_MASK; // CLOCK PIT
	PIT->MCR = 0x0;	// turn on PIT
	PIT->CHANNEL[0].LDVAL = 0x0100000;
	PIT->CHANNEL[0].TCTRL  = 3; //|= (1 << 28) | (1<<29) | (1<<30);	
	
	/*Set up Timer B (tracks real time elapsed*/
	//Use a PIT timer, every milisecond it generates an interrupt 
	
	PIT->CHANNEL[1].LDVAL = 0x20900; //one milisecond
	PIT->CHANNEL[1].TCTRL = 3; //enable timer

	NVIC_EnableIRQ(PIT0_IRQn); //Enable interrupts 0!!!!!!!!!!!
	NVIC_EnableIRQ(PIT1_IRQn); //Enable interrupts 1!!!!!!!!!!!
	
	NVIC_SetPriority(SVCall_IRQn, 1);
	NVIC_SetPriority(PIT0_IRQn, 1);
	NVIC_SetPriority(PIT1_IRQn, 0);
	
	current_time=(realtime_t*)malloc(sizeof(realtime_t));
	
	current_time->sec=0;
	current_time->msec=0;
	process_begin();	/* In assembly, actually launches processes */
	//__disable_irq();
}


void PIT1_IRQHandler(void)
	//the PIT1 timer should check the waiting queue to see if any processes have arrived, if they have move it from
	//waiting queue to realtime ready queue
{
	PIT->CHANNEL[1].TCTRL &= ~PIT_TCTRL_TEN_MASK; //disabling the timer so that a new value can be loaded
	
	if(current_time->msec<1000){
	current_time->msec+=1;
	}else{
	current_time->sec+=1;
	current_time->msec=0;
	}
	
	
	if(process_waiting_rt!=NULL){
	process_t* process= take_from_head(&process_waiting_rt);
	if((process->arrival_time->msec+ process->arrival_time->sec*1000)<(current_time->msec+current_time->sec*1000)){
	add_to_tail(&process_queue_rt,process,process->deadline);
	}else{
	add_to_tail(&process_waiting_rt,process,process->arrival_time);
	}	
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
	/* New things added:
	*Implements two-level scheduling
	*There is a real-time scheduling queue that has a higher priority than
	*the normal ready queue used for ordinary concurrent processes.
	*
	*Keeps track of the number of tasks that missed their deadlines in a
	global variable process_deadline_miss.
	*/
	/* cursp==0 -> No process currently running */
	
	
	
	if (cursp == 0) {	
		if(current_process->deadline != NULL){
			if((current_process->deadline->msec+ current_process->deadline->sec*1000)<(current_time->msec+current_time->sec*1000)){ //check if a process met its deadline
					process_deadline_miss++;
				}
		}
		current_process = NULL;
		if (process_queue == NULL && process_queue_rt == NULL && process_waiting_rt == NULL) { 
			__disable_irq();
		  return 0; 
		}	/* No processes left */
		else {
			if(process_queue_rt != NULL){ //check if there are processes on the real time queue
				PIT->CHANNEL[0].TCTRL = 1; // Disable PIT0
				__enable_irq(); // Enable global interrupts
				// your busy-wait code here

				current_process = take_from_head(&process_queue_rt);
				
				// Disable global interrupts
				__disable_irq();
				PIT->CHANNEL[0].TCTRL = 3; // Enable PIT0

				return current_process->sp;
			}else {
				current_process = take_from_head(&process_queue);
				return current_process->sp;
			}
		}
	}
	
	/* cursp != 0 -> Some running process was interrupted */
	else {
		if(current_process->deadline!=NULL){
			current_process->sp=cursp;
			
			
			
			PIT->CHANNEL[0].TCTRL = 1; // Disable PIT0
				__enable_irq(); // Enable global interrupts
			add_to_tail(&process_queue_rt, current_process, current_process->deadline);
			current_process=take_from_head(&process_queue_rt);
			__disable_irq();
				PIT->CHANNEL[0].TCTRL = 3; // Enable PIT0
			
		 return current_process->sp;
		}else{
		/* Save running process SP and add back to process queue to run later */
		current_process->sp = cursp;
		add_to_tail(&process_queue, current_process, NULL);
		
		/* Return next process from queue */
		current_process = take_from_head(&process_queue);
		return current_process->sp;
		}
	}
}
/*-----------------------------------------
 *Function for Lab 5
 *-----------------------------------------*/

realtime_t* add(realtime_t *one, realtime_t*two){
	
	realtime_t* sum;
	
	if(one->msec+two->msec>999){
		sum->msec= (one->msec + two->msec) -1000;
		sum->sec+=1;
	}else{
		sum->msec = one->msec + two->msec;
	}
	sum->sec=one->sec+two->sec;
	
	
	return sum;
	
}	
int process_rt_create(void (*f)(void), int n, realtime_t *start, realtime_t *work, realtime_t *deadline){
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
	
	//setting arrival_time
	
	new_proc->arrival_time=start;

	//setting deadline
	//Adding this wrong, what if the combined millisecs are too high? 
	new_proc->deadline=add(deadline,new_proc->arrival_time);
	if(start->msec==0 && start->sec==0){
	add_to_tail(&process_queue_rt,new_proc,new_proc->deadline);
	}else{	
	add_to_tail(&process_waiting_rt, new_proc, new_proc->arrival_time);
	}
	return 0;	/* Successfully created process and bookkeeping */
	//end stuff taken from process_create
	
	
}
