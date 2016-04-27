/*************************************************************************
 * Lab 5 test for only normal processes. No real time processes
 * 
 * 
 * 
 ************************************************************************/
 
#include "utils.h"
#include "3140_concur.h"

/*------------------*/
/* Helper functions */
/*------------------*/
void shortDelay(){delay();}
void mediumDelay() {delay(); delay();}



/*----------------------------------------------------
 * Non real-time process
 *   Blinks red LED 10 times. 
 *   Should be blocked by real-time process at first.
 *----------------------------------------------------*/
 
void pNRT(void) {
	int i;
	for (i=0; i<4;i++){
	LEDRed_On();
	shortDelay();
	LEDRed_Toggle();
	shortDelay();
	}
	
}

/*--------------------------------------------*/
/* Main function - start concurrent execution */
/*--------------------------------------------*/
int main(void) {	
	 
	LED_Initialize();

    /* Create processes */ 
    if (process_create(pNRT, NRT_STACK) < 0) { return -1; }
   
    /* Launch concurrent execution */
	process_start();

  LEDGreen_On();
	
	/* Hang out in infinite loop (so we can inspect variables if we want) */ 
	while (1);
	return 0;
}
