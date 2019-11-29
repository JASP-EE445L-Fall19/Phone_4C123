// I2CTestMain.c
// Runs on LM4F120/TM4C123
// Test the functions provided in I2C0.c by periodically sampling
// the temperature, parsing the result, and sending it to the UART.
// Daniel Valvano
// May 3, 2015

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2014
   Section 8.6.4 Programs 8.5, 8.6 and 8.7

 Copyright 2014 by Jonathan W. Valvano, valvano@mail.utexas.edu
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
// I2C0SCL connected to PB2 and to pin 4 of HMC6352 compass or pin 3 of TMP102 thermometer
// I2C0SDA connected to PB3 and to pin 3 of HMC6352 compass or pin 2 of TMP102 thermometer
// SCL and SDA lines pulled to +3.3 V with 10 k resistors (part of breakout module)
// ADD0 pin of TMP102 thermometer connected to GND
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "Drivers/inc/PCF8523.h"
#include "Periphs/inc/PLL.h"
#include "Periphs/inc/UART.h"
// For debug purposes, this program may peek at the I2C0 Master
// Control/Status Register to try to provide a more meaningful
// diagnostic message in the event of an error.  The rest of the
// interface with the I2C hardware occurs through the functions
// in I2C0.c.
#define I2C0_MASTER_MCS_R       (*((volatile unsigned long *)0x40020004))

// DEBUGPRINTS==0 configures for no test prints, other value prints test text
// This tests the math used to convert the raw temperature value
// from the thermometer to a string that is displayed.  Verify
// that the left and right columns are the same.
#define DEBUGPRINTS 1
// DEBUGWAIT is time between test prints as a parameter for the Delay() function
// DEBUGWAIT==16,666,666 delays for 1 second between lines
// This is useful if the computer terminal program has limited
// screen or log space to prevent the text from scrolling too
// fast.
// Definition has no effect if test prints are off.
#define DEBUGWAIT   16666666

#define RTC_ADDR 		0x68			// slave addr for PCF
#define TIME_BASE		0x03			// Base addr


// delay function for testing from sysctl.c
// which delays 3*ulCount cycles
#ifdef __TI_COMPILER_VERSION__
  //Code Composer Studio Code
  void Delay(unsigned long ulCount){
  __asm (  "    subs    r0, #1\n"
      "    bne     Delay\n"
      "    bx      lr\n");
}

#else
  //Keil uVision Code
  __asm void
  Delay(unsigned long ulCount)
  {
    subs    r0, #1
    bne     Delay
    bx      lr
  }

#endif
// function parses raw 16-bit number from temperature sensor in form:
// rawdata[0] = 0
// rawdata[15:8] 8-bit signed integer temperature
// rawdata[7:4] 4-bit unsigned fractional temperature (units 1/16 degree C)
//  or
// rawdata[0] = 1
// rawdata[15:7] 9-bit signed integer temperature
// rawdata[6:3] 4-bit unsigned fractional temperature (units 1/16 degree C)
void parseTemp(unsigned short rawData, int * tempInt, int * tempFra){
  if(rawData&0x0001){  // 13-bit mode
    *tempInt = rawData>>7;
    if(rawData&0x8000){// negative value
      *tempFra = (16 - ((rawData>>3)&0xF))*10000/16;
                                                 // treat as 9-bit signed
      *tempInt = (*tempInt) - 1512;              // subtract 512 to make integer portion signed
                                                 // subtract extra 1,000 so integer portion is
                                                 // still negative in the case of -0.XXXX
                                                 // (never display thousands digit)
      if(((*tempFra) > 0) && (*tempFra) < 10000){// fractional part "borrows" from integer part
        *tempInt = (*tempInt) + 1;
      }
    }
    else{
      *tempFra = ((rawData>>3)&0xF)*10000/16;
    }
  }
  else{
    *tempInt = rawData>>8;
    if(rawData&0x8000){// negative value
      *tempFra = (16 - ((rawData>>4)&0xF))*10000/16;
                                                 // treat as 8-bit signed
      *tempInt = (*tempInt) - 1256;              // subtract 256 to make integer portion signed
                                                 // subtract extra 1,000 so integer portion is
                                                 // still negative in the case of -0.XXXX
                                                 // (never display thousands digit)
      if(((*tempFra) > 0) && (*tempFra) < 10000){// decimal part "borrows" from integer part
        *tempInt = (*tempInt) + 1;
      }
    }
    else{
      *tempFra = ((rawData>>4)&0xF)*10000/16;
    }
  }
}
// function sends temperature integer and decimal components to UART
// in form:
// XXX.XXXX or -XXX.XXXX
// tempInt is signed integer value of temperature
// tempFra is unsigned fractional value of temperature (units 1/10000 degree C)
void displayTemp(int * tempInt, int * tempFra){
  uint32_t index = 0;               // string index
  char str[10];                           // holds 9 characters
  // first character is '-' if negative
  if((*tempInt) < 0){
    *tempInt = -1*(*tempInt);
    str[index] = '-';
    index = index + 1;
  }
  // next character is hundreds digit if not zero
  if(((*tempInt)/100)%10 != 0){
    str[index] = (((*tempInt)/100)%10) + '0';
    index = index + 1;
  // hundreds digit is not zero so next character is tens digit
    str[index] = (((*tempInt)/10)%10) + '0';
    index = index + 1;
  }
  // hundreds digit is zero so next character is tens digit only if not zero
  else if(((*tempInt)/10)%10 != 0){
    str[index] = (((*tempInt)/10)%10) + '0';
    index = index + 1;
  }
  // next character is ones digit
  str[index] = ((*tempInt)%10) + '0';
  index = index + 1;
  // next character is '.'
  str[index] = '.';
  index = index + 1;
  // next character is tenths digit
  str[index] = (((*tempFra)/1000)%10) + '0';
  index = index + 1;
  // next character is hundredths digit
  str[index] = (((*tempFra)/100)%10) + '0';
  index = index + 1;
  // next character is thousandths digit
  str[index] = (((*tempFra)/10)%10) + '0';
  index = index + 1;
  // next character is ten thousandths digit
  str[index] = ((*tempFra)%10) + '0';
  index = index + 1;
  // fill in any remaining characters with ' '
  while(index < 9){
    str[index] = ' ';
    index = index + 1;
  }
  // final character is null terminater
  str[index] = 0;
  // send string to UART
  UART_OutChar('\r');
  UART_OutString(str);
}

void itoa(char* string, uint32_t val) {
	if (val < 10) {
		string[0] = val + '0';
		string[1] = 0;
		return;
	}
	itoa(string + 1, val / 10);
	string[0] = val % 10;
}


extern int status;
extern int ack_ct;

uint8_t RTC_Buf[10];

int stat;
int seconds, minutes;
int i = 0;
//volatile uint16_t heading = 0;
//volatile uint8_t controlReg = 0;
int main(void){
  PLL_Init(Bus80MHz);
  UART_Init(5);
	UART_OutString("Example I2C");
  PCF8523_I2C0_Init();
   
	while(1){
  //test display and number parser (can safely be skipped)
	#if DEBUGPRINTS
		//i = (i + 1)%100;
		// Send Code
		if (i == 1) {
			uint8_t arr[3] = {0x50, 0x59, 0x23};
			stat = I2C_RTC_Send_List(RTC_ADDR, TIME_BASE, arr, 3);		// Send initial time
			UART_OutString("Send Stat: ");
			UART_OutUDec(stat);
			UART_OutString(" ");
			UART_OutString("Ack Ct: ");
			UART_OutUDec(ack_ct);
			UART_OutString("     ");
		}
		
		int err_code = I2C_RTC_Recv_List(RTC_ADDR, TIME_BASE, RTC_Buf, 3);
	
		char sec_arr[3], min_arr[3], hr_arr[3];
		bin2bcd(RTC_Buf[0], sec_arr);
		bin2bcd(RTC_Buf[1], min_arr);
		bin2bcd(RTC_Buf[2], hr_arr);
		/* Debug Prints */
		UART_OutString("Recv Sec: ");
		UART_OutString(sec_arr);
		UART_OutString(" ");
		UART_OutString("Recv Min: ");
		UART_OutString(min_arr);
		UART_OutString(" ");
		UART_OutString("Recv Hr: ");
		UART_OutString(hr_arr);
		UART_OutString(" ");
		UART_OutString("Err code: ");
		UART_OutUDec(err_code);
		UART_OutString(" ");
		UART_OutString("Ack Ct: ");
		UART_OutUDec(ack_ct);
		UART_OutString("\r\n");
		
		Delay(DEBUGWAIT/2);
	#endif
  }
/*  while(1){
                                          // write commands to 0x42
    I2C_Send1(0x42, 'A');                 // use command 'A' to sample
    Delay(100000);                        // wait 6,000 us for sampling to finish
                                          // read from 0x43 to get data
    heading = I2C_Recv2(0x43);            // 0 to 3599 (units: 1/10 degree)
// test sending multiple bytes and receiving single byte
                                          // write commands to 0x42
    I2C_Send2(0x42, 'g', 0x74);           // use command 'g' to read from RAM 0x74
    Delay(1167);                          // wait 70 us for RAM access to finish
    controlReg = I2C_Recv(0x43);          // expected 0x50 as default
    Delay(16666666);                      // wait 1 sec
  }*/
}
