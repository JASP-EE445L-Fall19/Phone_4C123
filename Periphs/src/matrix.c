// Matrix.c
// Runs on  LM4F120/TM4C123
// Provide functions that initialize GPIO ports and SysTick 
// Use periodic polling
// Daniel Valvano
// August 11, 2014

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2015

   Example 5.4, Figure 5.18, Program 5.13

Copyright 2015 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */

// PF3 connected to column 3 (keypad pin 4) using 10K pull-up
// PF2 connected to column 2 (keypad pin 3) using 10K pull-up
// PF1 connected to column 1 (keypad pin 2) using 10K pull-up
// PF0 connected to column 0 (keypad pin 1) using 10K pull-up
// PE3 connected to row 3 (keypad pin 8)
// PE2 connected to row 2 (keypad pin 7)
// PE1 connected to row 1 (keypad pin 6) (remove R9, R10)
// PE0 connected to row 0 (keypad pin 5) (remove R9, R10)

// [1] [2] [3] [A]
// [4] [5] [6] [B]
// [7] [8] [9] [C]
// [*] [0] [#] [D]
// Pin1 . . . . . . . . Pin8
// Pin 1 -> Column 0 (column starting with 1)
// Pin 2 -> Column 1 (column starting with 2)
// Pin 3 -> Column 2 (column starting with 3)
// Pin 4 -> Column 3 (column starting with A)
// Pin 5 -> Row 0 (row starting with 1)
// Pin 6 -> Row 1 (row starting with 4)
// Pin 7 -> Row 2 (row starting with 7)
// Pin 8 -> Row 3 (row starting with *)
#include <stdint.h>
#include <string.h>
#include "../inc/FIFO.h"
#include "../../../inc/tm4c123gh6pm.h"

void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode
volatile uint32_t Counts = 0;

#define FIFOSIZE   16         // size of the FIFOs (must be power of 2)
#define FIFOSUCCESS 1         // return value on success
#define FIFOFAIL    0         // return value on failure
                              // create index implementation FIFO (see FIFO.h)
AddIndexFifo(Matrix, 16, char, 1, 0) // create a FIFO
uint32_t HeartBeat;  // incremented every 25 ms

// Initialize Systick periodic interrupts
// Units of period are 20ns
// Range is up to 2^24-1
/*void SysTick_Init(uint32_t period){
  NVIC_ST_CTRL_R = 0;         // disable SysTick during setup
  NVIC_ST_RELOAD_R = period - 1;// reload value
  NVIC_ST_CURRENT_R = 0;      // any write to current clears it
  NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R&0x00FFFFFF)|0x40000000; // priority 2
                              // enable SysTick with core clock and interrupts
  NVIC_ST_CTRL_R = NVIC_ST_CTRL_ENABLE+NVIC_ST_CTRL_CLK_SRC+NVIC_ST_CTRL_INTEN;
  EnableInterrupts();
}*/

void Timer2A_MatrixCheck_Init(int32_t period){
	volatile uint32_t delay;
  SYSCTL_RCGCTIMER_R |= 0x04;   // 0) activate timer2
  delay = SYSCTL_RCGCTIMER_R;          // user function
	volatile int delay2 = 2;
	delay2++;
	delay2++;
  TIMER2_CTL_R = 0x00000000;    // 1) disable timer2A during setup
  TIMER2_CFG_R = 0x00000000;    // 2) configure for 32-bit mode
  TIMER2_TAMR_R = 0x00000002;   // 3) configure for periodic mode, default down-count settings
  TIMER2_TAILR_R = period - 1;    // 4) reload value
  TIMER2_TAPR_R = 0;            // 5) bus clock resolution
  TIMER2_ICR_R = 0x00000001;    // 6) clear timer2A timeout flag
  TIMER2_IMR_R = 0x00000001;    // 7) arm timeout interrupt
  NVIC_PRI5_R = (NVIC_PRI5_R&0x00FFFFFF)|0x40000000; // 8) priority 4
// interrupts enabled in the main program after all devices initialized
// vector number 39, interrupt number 23
  NVIC_EN0_R = 1<<23;           // 9) enable IRQ 23 in NVIC
  TIMER2_CTL_R = 0x00000001;    // 10) enable timer2A
}

// Initialization of Matrix keypad
void MatrixKeypad_Init(void){ 
  SYSCTL_RCGCGPIO_R |= 0x0030;        // enable Ports A and D
  HeartBeat = 0;
	GPIO_PORTF_LOCK_R = 0x4C4F434B;   // 2) unlock GPIO Port F
  GPIO_PORTF_CR_R = 0x1F;           // allow changes to PF4-0
  GPIO_PORTF_DEN_R |= 0x0F;        // enable digital I/O on PA5-2
  GPIO_PORTF_DIR_R &= ~0x0F;       // make PA5-2 in (PA5-2 columns)
  GPIO_PORTF_PCTL_R = (GPIO_PORTF_PCTL_R&0xFFFF0000)+0x00000000;
  GPIO_PORTF_AFSEL_R = 0;     // disable alternate functionality on PA
  GPIO_PORTF_AMSEL_R = 0;     // disable analog functionality on PA
  GPIO_PORTE_DATA_R &= ~0x0F;      // DIRn=0, OUTn=HiZ; DIRn=1, OUTn=0
  GPIO_PORTE_DEN_R |= 0x0F;        // enable digital I/O on PD3-0
  GPIO_PORTE_DIR_R &= ~0x0F;       // make PD3-0 in (PD3-0 rows)
  GPIO_PORTE_PCTL_R = (GPIO_PORTE_PCTL_R&0xFFFF0000)+0x00000000;
  GPIO_PORTE_AMSEL_R = 0;     // disable analog functionality on PD
  GPIO_PORTF_AFSEL_R = 0;     // disable alternate functionality on PD
}



struct Row{
  uint32_t direction;
  char keycode[4];};
typedef const struct Row RowType;
RowType ScanTab[5]={
{   0x01, "123A" }, // row 0
{   0x02, "456B" }, // row 1
{   0x04, "789C" }, // row 2
{   0x08, "*0#D" }, // row 3
{   0x00, "    " }};

/* Returns ASCII code for key pressed,
   Num is the number of keys pressed
   both equal zero if no key pressed */
char MatrixKeypad_Scan(int32_t *Num){
  RowType *pt;
  char column, key;
  int32_t j;
  (*Num) = 0;
  key = 0;    // default values
  pt = &ScanTab[0];
  while(pt->direction){
    GPIO_PORTE_DIR_R = pt->direction;      // one output
    GPIO_PORTE_DATA_R &= ~0x0F;            // DIRn=0, OUTn=HiZ; DIRn=1, OUTn=0
    for(j=1; j<=10; j++);                  // very short delay
    column = ((GPIO_PORTF_DATA_R&0x0F));// read columns
    for(j=0; j<=2; j++){
      if((column&0x01)==0){
        key = pt->keycode[j];
        (*Num)++;
      }
      column>>=1;  // shift into position
    }
    pt++;
  }
  return key;
}

char static LastKey; 
void Matrix_Init(void){
  LastKey = 0;             // no key typed
  MatrixFifo_Init();
  MatrixKeypad_Init();     // Program 4.13
  Timer2A_MatrixCheck_Init(25*80000);//SysTick_Init(25*80000);  // Program 5.12, 25 ms polling
} 

void Timer2A_Handler(void) {
//void SysTick_Handler(void){  
	TIMER2_ICR_R = TIMER_ICR_TATOCINT; // acknowledge
	char thisKey; int32_t n;
  thisKey = MatrixKeypad_Scan(&n); // scan 
  if((thisKey != LastKey) && (n == 1)){
    MatrixFifo_Put(thisKey);
    LastKey = thisKey;
  } else if(n == 0){
    LastKey = 0; // invalid
  }
  HeartBeat++;
}
// input ASCII character from keypad
// spin if Fifo is empty
char Matrix_InChar(void){  char letter;
  if (MatrixFifo_Get(&letter) == FIFOFAIL)
		return 0;
  return(letter);
}

typedef struct {
	char num;
	char chars[10];
} Numpad_Schema;

Numpad_Schema numSchema[10] = { {'0', {" .'!?*"}},
																//{'1', {" 1"}},
																{'2', {"abcABC2"}},
																{'3', {"defDEF3"}},
															  {'4', {"ghiGHI4"}},
																{'5', {"jklJKL5"}},
																{'6', {"mnoMNO6"}},
																{'7', {"pqrsPQRS7"}},
																{'8', {"tuvTUV8"}},
																{'9', {"wxyzWXYZ9"}}
															};

int curIndex = 0;
int prevInput = -1;
/* Takes the numpad numbers and converts it to text*/
char numpad2TextInput(char input, int* deletePrev) {
	char convertedInput = 0;
	*deletePrev = 0;
	/* '1' breaks the chain*/
	if (input == '1' && input == prevInput) {
		//curIndex = 0;
		//if (input == prevInput)
			*deletePrev = 1;
	}
/* To move to another input */
	if (prevInput != input) {
		curIndex = 0;
		prevInput = input;
	}
	
	for(int i = 0; i < 10; i++) {
		if (numSchema[i].num == input) {
			convertedInput = numSchema[i].chars[curIndex];
			if (curIndex)
				*deletePrev = 1;
			curIndex = (curIndex + 1) % strlen(numSchema[i].chars);
			break;
		}
	}
	return convertedInput;
}
