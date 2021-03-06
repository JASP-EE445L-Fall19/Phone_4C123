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
#include "../inc/tm4c123gh6pm.h"

#include "Drivers/inc/PCF8523.h"
#include "Drivers/inc/SIM800H.h"

#include "Periphs/inc/PLL.h"
#include "Periphs/inc/UART_Putty.h"
#include "Periphs/inc/ILI9341.h"
#include "Periphs/inc/matrix.h"

#include "UI/UI_Components.h"
#include "../lvgl/lvgl.h"

#include "Bitmaps/Longhorn.h"
#include "Bitmaps/Call_icon.h"
#include "Bitmaps/Text_icon.h"


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
#define TEST_GSM  0
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
	TEXT_SCREEN,
	CALL_BUSY_SCREEN,
	TEXT_BUSY_SCREEN
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
int chooseBox = 0;
char* phoneNumber, *textMessageString;
	
int textQueued = 0;

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

lv_obj_t* phoneLabel, *textLabel, *callTextBtn;

/* MAIN DISPLAY FUNCTIONS */
lv_obj_t *call_btn, *text_btn, *time_field, *mainText;
void mainDisplay() {
		call_btn = createCallIcon(&Call_icon);
		text_btn = createTextIcon(&Text_icon);
		time_field = createTime("Time: TBD", 20, 20, 200, 60);
		mainText = createMainText("JASP: Use it and Gasp!");
}

void mainDelete() {
	lv_obj_del(mainText);
	lv_obj_del(text_btn);
	lv_obj_del(call_btn);
	lv_obj_del(time_field);
}
/* ************ */

/* CALL DISPLAY FUNCTION */
lv_obj_t* phoneTextArea, *textMessageArea;
void callDisplay() {
	phoneTextArea = createPhoneTextArea();
	lv_ta_set_cursor_type(phoneTextArea, LV_CURSOR_BLOCK);
	
	callTextBtn = createCallTextButton("Call (*)");
	return;
}

void callDelete() {
	lv_obj_del(phoneTextArea);
	lv_obj_del(callTextBtn);
}
/* ************ */


/* TEXT DISPLAY FUNCTION */
void textDisplay() {
	phoneTextArea = createPhoneTextArea();
	lv_ta_set_cursor_type(phoneTextArea, LV_CURSOR_BLOCK);
	
	textMessageArea = createTextMessageArea();
	callTextBtn = createCallTextButton("Text (*)");
	return;
}

void textDelete() {
	lv_obj_del(phoneTextArea);
	lv_obj_del(textMessageArea);
	lv_obj_del(callTextBtn);
	chooseBox = 0;
}
/* ************ */


lv_obj_t* main_fn_text;
/* CALL BUSY DISPLAY FUNCTIONS */
void callBusyDisplay() {
	char pt[25] = "Calling ";
	main_fn_text = createMainText(strcat(pt, phoneNumber));
}
void callBusyDelete() {
	lv_obj_del(main_fn_text);
}
/**********/


/* TEXT BUSY DISPLAY FUNCTIONS */
void textBusyDisplay() {
	char pt[25] = "Texting ";
	main_fn_text = createMainText(strcat(pt, phoneNumber));
}
void textBusyDelete() {
	lv_obj_del(main_fn_text);
}
/**********/



void switchTextBox() {
	if (chooseBox ^= 1) {			// choose textMessageArea
		lv_ta_set_cursor_type(phoneTextArea, LV_CURSOR_NONE);
		lv_ta_set_cursor_type(textMessageArea, LV_CURSOR_BLOCK);
	
	}
	else {
		//lv_ta_set_cursor_type(phoneTextArea, LV_CURSOR_BLOCK);
		//lv_ta_set_cursor_type(textMessageArea, LV_CURSOR_NONE);
		nextScreen = TEXT_BUSY_SCREEN;
		phoneNumber = lv_ta_get_text(phoneTextArea);
		textMessageString = lv_ta_get_text(textMessageArea);
		isDisplayed = 0;
		textQueued = 1;
	}
}

void (*renderScreen[])() = {mainDisplay, callDisplay, textDisplay, callBusyDisplay, textBusyDisplay};	
void (*deleteScreen[])() = {mainDelete, callDelete, textDelete, callBusyDelete, textBusyDelete};


void handleInput(char input) {
	if (!input && !textQueued) 
		return;
	/* Main Screen Handler */
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
	
	/* Call Screen Handler */
	else if (curScreen == CALL_SCREEN) {
		if (input == '#') {
			nextScreen = MAIN_SCREEN;
			isDisplayed = 0;
		}
		else if (input == '*') {
			nextScreen = CALL_BUSY_SCREEN;
			textQueued = 1;
			phoneNumber = lv_ta_get_text(phoneTextArea);
			isDisplayed = 0;
		}
		else {
			lv_ta_add_char(phoneTextArea, input);
			isDisplayed = 0;
		}
			
	}
	
	/* Text Screen Handler */
	else if (curScreen == TEXT_SCREEN) {
		if (input == '#') {
			nextScreen = MAIN_SCREEN;
			isDisplayed = 0;
		}
		else if (input == '*') {
			switchTextBox();
		}
		else {
			if (chooseBox) {
				// Text Message Box
				int del_prev;
				char disp_char = numpad2TextInput(input, &del_prev);
				if (del_prev)
					lv_ta_del_char(textMessageArea);
				if (disp_char)
					lv_ta_add_char(textMessageArea, disp_char);
			}
			else
				// Phone NumberBox
				lv_ta_add_char(phoneTextArea, input);
		}
	}
	
	/* CALL BUSY HANDLER */
	else if (curScreen == CALL_BUSY_SCREEN) {
		if (textQueued) {
			/* Call Person*/ 
			SIM800H_SetAudio();
			SIM800H_SetSpeakerVolume();
			SIM800H_SetMicVolume();
			SIM800H_CallPhone(phoneNumber);
			textQueued = 0;
		}
		if (input == '#') {
			nextScreen = MAIN_SCREEN;
			/* Hang up phone */ 
			SIM800H_HangUpPhone();
			//callTextBusy = 0;
			isDisplayed = 0;
		}
	}

	/* TEXT BUSY HANDLER */
	else if (curScreen == TEXT_BUSY_SCREEN) {
		if (textQueued) {
			/* Text Person */
			char phone_schema[15] = "\"";
			strcat(strcat(phone_schema, phoneNumber), "\"\r");
			SIM800H_SendText(phone_schema, textMessageString);
			lv_label_set_text(main_fn_text, "Text was successful!");
			isDisplayed = 0;
			textQueued = 0;
		}
		if (input == '#') {
			nextScreen = MAIN_SCREEN;
			isDisplayed = 0;
		}
		
	}
		
	return;
}


int main(void){
  PLL_Init(Bus80MHz);
	#if TEST_GSM
		UART0_Init(5);
		UART0_OutString("Hello. A longer string");
		SIM800H_Init();
		SIM800H_SendText("\"5127431885\"\r", "This is Arjun with a working GSM!");
		while(1) {
		};
	#else
	SIM800H_Init();
	UART0_Init(5);
	UART0_OutString("Example I2C");
	PCF8523_I2C0_Init();
	Timer1_ClockUpdate_Init(10000000);
	/* LittleVGL */
	ILI9341_InitR(INITR_BLACKTAB);
	LittlevGL_Init();
	Matrix_Init();
	mainDisplay();
	#if SET_DATE_TIME
		// Send Code
			dateTime.seconds = 0x00;
			dateTime.minutes = 0x43; 
			dateTime.hours = 0x05;
			dateTime.date = 5;
			dateTime.day_int = 4;
			dateTime.month_int = 0x11;
			dateTime.year = 0;
		
			stat = setTimeAndDate(&dateTime);		// Send initial time
			UART0_OutString("Send Stat: ");
			UART0_OutUDec(stat);
			UART0_OutString(" ");
			UART0_OutString("Ack Ct: ");
			UART0_OutUDec(ack_ct);
			UART0_OutString("     ");
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
		UART0_OutString("Ctl: ");
		UART0_OutUDec(ctl);
		UART0_OutString(" ");
		*/
		int err_code = getTimeAndDate(&dateTime);
	
		char sec_arr[3], min_arr[3], hr_arr[3];
		bcd2arr(dateTime.seconds, sec_arr);
		bcd2arr(dateTime.minutes, min_arr);
		bcd2arr(dateTime.hours, hr_arr);
		/* Debug Prints */
		UART0_OutString("Recv Sec: ");
		UART0_OutString(sec_arr);
		UART0_OutString(" ");
		UART0_OutString("Recv Min: ");
		UART0_OutString(min_arr);
		UART0_OutString(" ");
		UART0_OutString("Recv Hr: ");
		UART0_OutString(hr_arr);
		UART0_OutString(" ");
		UART0_OutString("Err code: ");
		UART0_OutUDec(err_code);
		UART0_OutString(" ");
		UART0_OutString("Ack Ct: ");
		UART0_OutUDec(ack_ct);
		UART0_OutString("\r\n");
		
		Delay(DEBUGWAIT/2);
	#endif
  }
	#endif
}
