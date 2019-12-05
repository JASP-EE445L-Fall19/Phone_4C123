// UART5.c
// Runs on LM4F120/TM4C123
// Use UART55 to implement bidirectional data transfer to and from a
// computer running HyperTerminal.  This time, interrupts and FIFOs
// are used.
// Daniel Valvano
// September 11, 2013
// Modified by EE345L students Charlie Gough && Matt Hawk
// Modified by EE345M students Agustinus Darmawan && Mingjie Qiu

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2015
   Program 5.11 Section 5.6, Program 3.10

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

// U0Rx (VCP receive) connected to PA0
// U0Tx (VCP transmit) connected to PA1
#include <stdint.h>
#include "../../../inc/tm4c123gh6pm.h"

#include "../inc/UART.h"
#include "../inc/Fifo_Custom.h"




void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode

// Initialize UART55
// Baud rate is 115200 bits/sec
void UART5_Init(int baudrate){
  SYSCTL_RCGCUART_R |= 0x20;            // activate UART55
  SYSCTL_RCGCGPIO_R |= 0x10;            // activate port A
  while((SYSCTL_PRGPIO_R & 0x10) == 0) {};
		
	UART5_CTL_R &= ~0x01;      // disable UART5
  //UART55_IBRD_R = 27;                    // IBRD = int(50,000,000 / (16 * 115,200)) = int(27.1267)
  //UART55_FBRD_R = 8;                     // FBRD = int(0.1267 * 64 + 0.5) = 8
                                        // 8 bit word length (no parity bits, one stop bit, FIFOs)
  UART5_IBRD_R = 80000000 / (16 * baudrate);		// Write IBRD
	UART5_FBRD_R = (80000000 % (16 * baudrate)) * 64 + ((16*115200)/2) / (16 * baudrate);
		UART5_LCRH_R = (UART5_LCRH_R & 0xFFFFFF8F) | (0x07<<4);
  //UART5_IFLS_R &= ~0x3F;                // clear TX and RX interrupt FIFO level fields
                                        // configure interrupt for TX FIFO <= 1/8 full
                                        // configure interrupt for RX FIFO >= 1/8 full
  //UART5_IFLS_R += (UART5_IFLS_TX1_8|UART5_IFLS_RX1_8);
	UART5_IFLS_R = 0x10;
                                        // enable TX and RX FIFO interrupts and RX time-out interrupt
  UART5_IM_R |= 0x10;//|UART5_IM_TXIM|UART5_IM_RTIM);
  UART5_CTL_R |= 0x301;                 // enable UART5
  UART5_ICR_R = 0;
	
	Fifo_Init();
	
	GPIO_PORTE_AFSEL_R |= 0x30;           // enable alt funct on PA1-0
  GPIO_PORTE_DEN_R |= 0x30;             // enable digital I/O on PA1-0
                                        // configure PA1-0 as UART5
  GPIO_PORTE_PCTL_R = (GPIO_PORTE_PCTL_R&0xFF00FFFF)+0x00110000;
  GPIO_PORTE_AMSEL_R = 0;               // disable analog functionality on PA
                                        // UART55=priority 2
  NVIC_PRI15_R = (NVIC_PRI15_R&0xFFFF00FF)|0x00004000; // bits 13-15
  NVIC_EN1_R |= (1 << 29);           // enable interrupt 5 in NVIC
  EnableInterrupts();
}

// input ASCII character from UART5
// spin if RxFifo is empty
char UART5_InChar(void){
	while((UART5_FR_R&0x10) != 0) {};
	return (char)(UART5_DR_R&0xFF);
}


// output ASCII character to UART5
// spin if TxFifo is full
void UART5_OutChar(char data){

	while((UART5_FR_R & 0x20) != 0) {}
	UART5_DR_R = data;
}
// at least one of three things has happened:
// hardware TX FIFO goes from 3 to 2 or less items
// hardware RX FIFO goes from 1 to 2 or more items
// UART5 receiver has timed out
void UART5_Handler(void){
	static char Data;
	while ((UART5_FR_R & 0x10) == 0) {
		Data = (char)(UART5_DR_R & 0xFF);
		Fifo_Put(Data);
	}
	UART5_ICR_R |= 0x10;

}

//------------UART5_OutString------------
// Output String (NULL termination)
// Input: pointer to a NULL-terminated string to be transferred
// Output: none
void UART5_OutString(char *pt){
  while(*pt){
    UART5_OutChar(*pt);
    pt++;
  }
}

//------------UART5_InUDec------------
// InUDec accepts ASCII input in unsigned decimal format
//     and converts to a 32-bit unsigned number
//     valid range is 0 to 4294967295 (2^32-1)
// Input: none
// Output: 32-bit unsigned number
// If you enter a number above 4294967295, it will return an incorrect value
// Backspace will remove last digit typed
uint32_t UART5_InUDec(void){
uint32_t number=0, length=0;
char character;
  character = UART5_InChar();
  while(character != CR){ // accepts until <enter> is typed
// The next line checks that the input is a digit, 0-9.
// If the character is not 0-9, it is ignored and not echoed
    if((character>='0') && (character<='9')) {
      number = 10*number+(character-'0');   // this line overflows if above 4294967295
      length++;
      UART5_OutChar(character);
    }
// If the input is a backspace, then the return number is
// changed and a backspace is outputted to the screen
    else if((character==BS) && length){
      number /= 10;
      length--;
      UART5_OutChar(character);
    }
    character = UART5_InChar();
  }
  return number;
}

//-----------------------UART5_OutUDec-----------------------
// Output a 32-bit number in unsigned decimal format
// Input: 32-bit number to be transferred
// Output: none
// Variable format 1-10 digits with no space before or after
void UART5_OutUDec(uint32_t n){
// This function uses recursion to convert decimal number
//   of unspecified length as an ASCII string
  if(n >= 10){
    UART5_OutUDec(n/10);
    n = n%10;
  }
  UART5_OutChar(n+'0'); /* n is between 0 and 9 */
}

//---------------------UART5_InUHex----------------------------------------
// Accepts ASCII input in unsigned hexadecimal (base 16) format
// Input: none
// Output: 32-bit unsigned number
// No '$' or '0x' need be entered, just the 1 to 8 hex digits
// It will convert lower case a-f to uppercase A-F
//     and converts to a 16 bit unsigned number
//     value range is 0 to FFFFFFFF
// If you enter a number above FFFFFFFF, it will return an incorrect value
// Backspace will remove last digit typed
uint32_t UART5_InUHex(void){
uint32_t number=0, digit, length=0;
char character;
  character = UART5_InChar();
  while(character != CR){
    digit = 0x10; // assume bad
    if((character>='0') && (character<='9')){
      digit = character-'0';
    }
    else if((character>='A') && (character<='F')){
      digit = (character-'A')+0xA;
    }
    else if((character>='a') && (character<='f')){
      digit = (character-'a')+0xA;
    }
// If the character is not 0-9 or A-F, it is ignored and not echoed
    if(digit <= 0xF){
      number = number*0x10+digit;
      length++;
      UART5_OutChar(character);
    }
// Backspace outputted and return value changed if a backspace is inputted
    else if((character==BS) && length){
      number /= 0x10;
      length--;
      UART5_OutChar(character);
    }
    character = UART5_InChar();
  }
  return number;
}

//--------------------------UART5_OutUHex----------------------------
// Output a 32-bit number in unsigned hexadecimal format
// Input: 32-bit number to be transferred
// Output: none
// Variable format 1 to 8 digits with no space before or after
void UART5_OutUHex(uint32_t number){
// This function uses recursion to convert the number of
//   unspecified length as an ASCII string
  if(number >= 0x10){
    UART5_OutUHex(number/0x10);
    UART5_OutUHex(number%0x10);
  }
  else{
    if(number < 0xA){
      UART5_OutChar(number+'0');
     }
    else{
      UART5_OutChar((number-0x0A)+'A');
    }
  }
}

//------------UART5_InString------------
// Accepts ASCII characters from the serial port
//    and adds them to a string until <enter> is typed
//    or until max length of the string is reached.
// It echoes each character as it is inputted.
// If a backspace is inputted, the string is modified
//    and the backspace is echoed
// terminates the string with a null character
// uses busy-waiting synchronization on RDRF
// Input: pointer to empty buffer, size of buffer
// Output: Null terminated string
// -- Modified by Agustinus Darmawan + Mingjie Qiu --
void UART5_InString(char *bufPt, uint16_t max) {
int length=0;
char character;
  character = UART5_InChar();
  while(character != CR){
    if(character == BS){
      if(length){
        bufPt--;
        length--;
        UART5_OutChar(BS);
      }
    }
    else if(length < max){
      *bufPt = character;
      bufPt++;
      length++;
      UART5_OutChar(character);
    }
    character = UART5_InChar();
  }
  *bufPt = 0;
}
