
#include "MK64F12.h"

/*----------------------------------------------------------------------------
  Function that initializes LEDs
 *----------------------------------------------------------------------------*/
void LED_Initialize(void) {

  SIM->SCGC5    |= (1 <<  10) | (1 <<  13);  /* Enable Clock to Port B & E */ 
  PORTB->PCR[22] = (1 <<  8) ;               /* Pin PTB22 is GPIO */
  PORTB->PCR[21] = (1 <<  8);                /* Pin PTB21 is GPIO */
  PORTE->PCR[26] = (1 <<  8);                /* Pin PTE26  is GPIO */
  
  PTB->PDOR = (1 << 21 | 1 << 22 );          /* switch Red/Green LED off  */
  PTB->PDDR = (1 << 21 | 1 << 22 );          /* enable PTB18/19 as Output */

  PTE->PDOR = 1 << 26;            /* switch Blue LED off  */
  PTE->PDDR = 1 << 26;            /* enable PTD1 as Output */
}

int red_on = 0;
int blue_on = 0;

/*----------------------------------------------------------------------------
  Function that toggles red LED
 *----------------------------------------------------------------------------*/

void LEDRed_Toggle (void) {
  PIT->CHANNEL[0].TCTRL = 1; //Disable timer to make this function atomic
  if (red_on) {
		PTB->PSOR   = 1 << 22;   /* Red LED Off*/
		red_on = 0;
	} else {
		PTB->PCOR   = 1 << 22;   /* Red LED On*/
		red_on = 1;
	}
	PIT->CHANNEL[0].TCTRL = 3;
}

/*----------------------------------------------------------------------------
  Function that toggles blue LED
 *----------------------------------------------------------------------------*/
void LEDBlue_Toggle (void) {
  PIT->CHANNEL[0].TCTRL = 1;
  if (blue_on) {
		PTB->PSOR   = 1 << 21;   /* Blue LED Off*/
		blue_on = 0;
	} else {
		PTB->PCOR   = 1 << 21;   /* Blue LED On*/
		blue_on = 1;
	}
	PIT->CHANNEL[0].TCTRL = 3;
}

/*----------------------------------------------------------------------------
  Function that turns on Red LED & all the others off
 *----------------------------------------------------------------------------*/
void LEDRed_On (void) {
  PIT->CHANNEL[0].TCTRL = 1;
  PTB->PCOR   = 1 << 22;   /* Red LED On*/
  PTB->PSOR   = 1 << 21;   /* Blue LED Off*/
  PTE->PSOR   = 1 << 26;   /* Green LED Off*/
  red_on      = 1;
  PIT->CHANNEL[0].TCTRL = 3;
  
}

/*----------------------------------------------------------------------------
  Function that turns on Green LED & all the others off
 *----------------------------------------------------------------------------*/
void LEDGreen_On (void) {
  PIT->CHANNEL[0].TCTRL = 1;
  PTB->PSOR   = 1 << 21;   /* Blue LED Off*/
  PTE->PCOR   = 1 << 26;   /* Green LED On*/
  PTB->PSOR   = 1 << 22;   /* Red LED Off*/
  PIT->CHANNEL[0].TCTRL = 3;
}

/*----------------------------------------------------------------------------
  Function that turns on Blue LED & all the others off
 *----------------------------------------------------------------------------*/
void LEDBlue_On (void) {
  PIT->CHANNEL[0].TCTRL = 1;
  PTE->PSOR   = 1 << 26;   /* Green LED Off*/
  PTB->PSOR   = 1 << 22;   /* Red LED Off*/
  PTB->PCOR   = 1 << 21;   /* Blue LED On*/
  blue_on     = 1;
  PIT->CHANNEL[0].TCTRL = 3;
}

/*----------------------------------------------------------------------------
  Function that turns all LEDs off
 *----------------------------------------------------------------------------*/
void LED_Off (void) {
  PIT->CHANNEL[0].TCTRL = 1;
  PTB->PSOR   = 1 << 22;   /* Green LED Off*/
  PTB->PSOR   = 1 << 21;     /* Red LED Off*/
  PTE->PSOR   = 1 << 26;    /* Blue LED Off*/
  PIT->CHANNEL[0].TCTRL = 3;
}

void delay(void){
	int j;
	for(j=0; j<1000000; j++);
}
