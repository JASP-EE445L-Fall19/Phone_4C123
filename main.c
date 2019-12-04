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
#include "Periphs/inc/ILI9341.h"
#include "../inc/tm4c123gh6pm.h"
#include "UI/UI_Components.h"
#include "../lvgl/lvgl.h"
#include "Bitmaps/Longhorn.h"
#include "Bitmaps/Call_icon.h"
#include "Bitmaps/Text_icon.h"

#include "Periphs/inc/matrix.h"
#include <string.h>

LV_IMG_DECLARE(Longhorn);
LV_IMG_DECLARE(Call_icon);
LV_IMG_DECLARE(Text_icon);

void lv_disp_buf_init(lv_disp_buf_t * disp_buf, void * buf1, void * buf2, uint32_t size_in_px_cnt);
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
#define DEBUGPRINTS 0
#define SET_DATE_TIME 0
// DEBUGWAIT is time between test prints as a parameter for the Delay() function
// DEBUGWAIT==16,666,666 delays for 1 second between lines
// This is useful if the computer terminal program has limited
// screen or log space to prevent the text from scrolling too
// fast.
// Definition has no effect if test prints are off.
#define DEBUGWAIT   16666666

#define RTC_ADDR 		0x68			// slave addr for PCF
#define TIME_BASE		0x03			// Base addr

enum screens {
	MAIN_SCREEN,
	CALL_SCREEN,
	TEXT_SCREEN
} curScreen = MAIN_SCREEN, nextScreen = MAIN_SCREEN;

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
	
extern int status;
extern int ack_ct;
DateTime dateTime;
int stat;
int i = 0;
extern int isDisplayed;
extern lv_obj_t* time_label;
int updateClock = 0;
char* full_time;
char* full_date;	
char* displayStr;
lv_obj_t* phoneLabel, *textLabel;

void Timer1_ClockUpdate_Init(uint32_t period){
  SYSCTL_RCGCTIMER_R |= 0x02;   // 0) activate TIMER1
  TIMER1_CTL_R = 0x00000000;    // 1) disable TIMER1A during setup
  TIMER1_CFG_R = 0x00000000;    // 2) configure for 32-bit mode
  TIMER1_TAMR_R = 0x00000002;   // 3) configure for periodic mode, default down-count settings
  TIMER1_TAILR_R = period - 1;    // 4) reload value
  TIMER1_TAPR_R = 0;            // 5) bus clock resolution
  TIMER1_ICR_R = 0x00000001;    // 6) clear TIMER1A timeout flag
  TIMER1_IMR_R = 0x00000001;    // 7) arm timeout interrupt
  NVIC_PRI5_R = (NVIC_PRI5_R&0xFFFF00FF)|0x00008000; // 8) priority 4
// interrupts enabled in the main program after all devices initialized
// vector number 37, interrupt number 21
  NVIC_EN0_R = 1<<21;           // 9) enable IRQ 21 in NVIC
  TIMER1_CTL_R = 0x00000001;    // 10) enable TIMER1A
}

void Timer1A_Handler(void) {
	TIMER1_ICR_R = TIMER_ICR_TATOCINT;
	updateClock = 1;
}

void getDisplayTime() {
	int err_code = getTimeAndDate(&dateTime);
	char sec_arr[3], min_arr[3], hr_arr[20], day_arr[30], date_arr[3], month_arr[12];
	bcd2arr(dateTime.seconds, sec_arr);
	bcd2arr(dateTime.minutes, min_arr);
	bcd2arr(dateTime.hours, hr_arr);
	bcd2arr(dateTime.date, date_arr);
	strcpy(day_arr, dateTime.day);
	strcpy(month_arr, dateTime.month);
			
	full_time = strcat(strcat(strcat(strcat(hr_arr, ":"), min_arr), ":"), sec_arr); 
	full_date = strcat(strcat(strcat(day_arr, date_arr), " "), month_arr);
	displayStr = strcat(strcat(full_date, "\n"), full_time); 
	lv_label_set_align(time_label, LV_LABEL_ALIGN_CENTER);
	lv_label_set_text(time_label, full_date);
}


lv_obj_t *call_btn, *text_btn, *time_field, *mainText;

void mainDisplay() {
		call_btn = createCallIcon(&Call_icon);
		text_btn = createTextIcon(&Text_icon);
		time_field = createTime("Time: TBD", 10, 20, 200, 60);
		mainText = createMainText("JASP: Use it and Gasp!");
}

void mainDelete() {
	lv_obj_del(mainText);
	lv_obj_del(text_btn);
	lv_obj_del(call_btn);
	lv_obj_del(time_field);
}

lv_obj_t* phoneTextArea, *textMessageArea;
void callDisplay() {
	phoneTextArea = createPhoneTextArea();
	phoneLabel = createPhoneLabel("Phone Number");
	return;
}
void textDisplay() {
	phoneTextArea = createPhoneTextArea();
	phoneLabel = createPhoneLabel("Phone Number");
	textMessageArea = createTextMessageArea();
	textLabel = createTextLabel("Message");
	return;
}


void callDelete() {
	lv_obj_del(phoneTextArea);
	lv_obj_del(phoneLabel);
}
void textDelete() {
	lv_obj_del(phoneTextArea);
	lv_obj_del(phoneLabel);
	lv_obj_del(textMessageArea);
	lv_obj_del(textLabel);
}

void (*renderScreen[])() = {mainDisplay, callDisplay, textDisplay};	
void (*deleteScreen[])() = {mainDelete, callDelete, textDelete};


int chooseBox = 0;
void handleInput(char input) {
	if (!input) 
		return;
	if (curScreen == MAIN_SCREEN) {
		if (input == '*') {
				nextScreen = CALL_SCREEN;
				isDisplayed = 0;
		}
		else if (input == '#') {
				nextScreen = TEXT_SCREEN;
				isDisplayed = 0;
		}
	}
	else if (curScreen == CALL_SCREEN) {
		if (input == '#') {
			nextScreen = MAIN_SCREEN;
			isDisplayed = 0;
		}
		else {
			lv_ta_add_char(phoneTextArea, input);
			isDisplayed = 0;
		}
			
	}
	else if (curScreen == TEXT_SCREEN) {
		if (input == '#') {
			nextScreen = MAIN_SCREEN;
			isDisplayed = 0;
		}
		else if (input == '*') {
			chooseBox ^= 1;
		}
		else {
			if (chooseBox)
				lv_ta_add_char(textMessageArea, input);
			else
				lv_ta_add_char(phoneTextArea, input);
		}
	}
	return;
}


int main(void){
  PLL_Init(Bus80MHz);
  UART_Init(5);
	UART_OutString("Example I2C");
	PCF8523_I2C0_Init();
	Timer1_ClockUpdate_Init(10000000);
	/* LittleVGL */
	ILI9341_InitR(INITR_BLACKTAB);
	LittlevGL_Init();
	Matrix_Init();
	mainDisplay();
	#if SET_DATE_TIME
		// Send Code
			dateTime.seconds = 0x15;
			dateTime.minutes = 0x31; 
			dateTime.hours = 0x16;
			dateTime.date = 3;
			dateTime.day_int = 2;
			dateTime.month_int = 0x11;
			dateTime.year = 0;
		
			stat = setTimeAndDate(&dateTime);		// Send initial time
			UART_OutString("Send Stat: ");
			UART_OutUDec(stat);
			UART_OutString(" ");
			UART_OutString("Ack Ct: ");
			UART_OutUDec(ack_ct);
			UART_OutString("     ");
	#endif
	while(1){
		if (curScreen != nextScreen) {
			lv_obj_clean(lv_scr_act());		// Clear screen
			(*deleteScreen[curScreen])();	// Delete components
			(*renderScreen[nextScreen])(); // Create components
			curScreen = nextScreen;
		}
		if (!isDisplayed) {
			lv_task_handler();
			isDisplayed = 1;
		}
		if (updateClock && curScreen == MAIN_SCREEN) {
			getDisplayTime();
			updateClock = 0;
			isDisplayed = 0;
		}
		char num_input = Matrix_InChar();
		handleInput(num_input);
		
		
  //test display and number parser (can safely be skipped)
	#if DEBUGPRINTS
		/*int ctl = I2C_RTC_Recv(RTC_ADDR, 0x02);
		UART_OutString("Ctl: ");
		UART_OutUDec(ctl);
		UART_OutString(" ");
		*/
		int err_code = getTimeAndDate(&dateTime);
	
		char sec_arr[3], min_arr[3], hr_arr[3];
		bcd2arr(dateTime.seconds, sec_arr);
		bcd2arr(dateTime.minutes, min_arr);
		bcd2arr(dateTime.hours, hr_arr);
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
}
