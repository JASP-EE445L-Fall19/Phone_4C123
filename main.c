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
#include "lvgl/lvgl.h"
#include "../inc/tm4c123gh6pm.h"

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

#define INC_TIME	  5
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
	

/* LITTLE VGL STUFF */	
void Timer0_Init(int32_t period){
	volatile int delay = 2;
	SYSCTL_RCGCTIMER_R |= 0x01;   // 0) activate TIMER0
  delay++;
	delay = SYSCTL_RCGCTIMER_R;
	TIMER0_CTL_R = 0x00000000;    // 1) disable TIMER0A during setup
  TIMER0_CFG_R = 0x00000000;    // 2) configure for 32-bit mode
  TIMER0_TAMR_R = 0x00000002;   // 3) configure for periodic mode, default down-count settings
  TIMER0_TAILR_R = period-1;    // 4) reload value
  TIMER0_TAPR_R = 0;            // 5) bus clock resolution
  TIMER0_ICR_R = 0x00000001;    // 6) clear TIMER0A timeout flag
  TIMER0_IMR_R = 0x00000001;    // 7) arm timeout interrupt
  NVIC_PRI4_R = (NVIC_PRI4_R&0x00FFFFFF)|0x80000000; // 8) priority 4
// interrupts enabled in the main program after all devices initialized
// vector number 35, interrupt number 19
  NVIC_EN0_R = 1<<19;           // 9) enable IRQ 19 in NVIC
  TIMER0_CTL_R = 0x00000001;    // 10) enable TIMER0A
}

int isDisplayed = 0;
void Timer0A_Handler(void) {
  TIMER0_ICR_R = TIMER_ICR_TATOCINT;// acknowledge timer0A timeout
	lv_tick_inc(INC_TIME);
	isDisplayed = 0;
}


static lv_disp_buf_t disp_buf;
static lv_color_t buf[LV_HOR_RES_MAX * 20];                     /*Declare a buffer for 10 lines*/
lv_disp_drv_t disp_drv;               /*Descriptor of a display driver*/

void my_disp_flush(lv_disp_t* disp, const lv_area_t* area, lv_color_t* color_p) {
	int32_t x, y;
    for(y = area->y1; y <= area->y2; y++) {
        for(x = area->x1; x <= area->x2; x++) {
            ILI9341_DrawPixel(x, 319 - y, vGL2ILI_Color(color_p->ch.red, color_p->ch.green, color_p->ch.blue));  /* Put a pixel to the display.*/
            color_p++;
        }
    }
    lv_disp_flush_ready(disp);         /* Indicate you are ready with the flushing*/
}

void LittlevGL_Init() {
	Timer0_Init(INC_TIME * 80000);
	lv_init();
	/* Set default theme */
	lv_theme_t * th = lv_theme_material_init(20, NULL);
	lv_theme_set_current(th);
	//lv_style_scr.body.main_color = LV_COLOR_RED;
	//lv_style_scr.body.grad_color = LV_COLOR_RED;
	
	lv_disp_buf_init(&disp_buf, buf, NULL, LV_HOR_RES_MAX * 20);    /*Initialize the display buffer*/
	lv_disp_drv_init(&disp_drv);          /*Basic initialization*/
	disp_drv.flush_cb = my_disp_flush;    /*Set your driver function*/
	disp_drv.buffer = &disp_buf;          /*Assign the buffer to the display*/
	lv_disp_drv_register(&disp_drv);      /*Finally register the driver*/
}

void createButton(char* text, int x, int y, int w, int h) {
	lv_obj_t* btn = lv_btn_create(lv_scr_act(), NULL);     /*Add a button the current screen*/
	lv_obj_set_pos(btn, x, y);                            /*Set its position*/
	lv_obj_set_size(btn, w, h);                          /*Set its size*/
	lv_obj_t * label = lv_label_create(btn, NULL);          /*Add a label to the button*/
	lv_label_set_text(label, text);                     /*Set the labels text*/
}

void createSlider() {
	lv_obj_t * slider = lv_slider_create(lv_scr_act(), NULL);
	lv_obj_set_width(slider, 40);                        /*Set the width*/
	lv_obj_align(slider, NULL, LV_ALIGN_CENTER, 0, 0);    /*Align to the center of the parent (screen)*/
}

int main(void){
  PLL_Init(Bus80MHz);
  UART_Init(5);
	UART_OutString("Example I2C");
	PCF8523_I2C0_Init();
	/* LittleVGL */
	ILI9341_InitR(INITR_BLACKTAB);
	//ILI9341_FillScreen(ILI9341_GREEN);
	//ILI9341_OutString("Hello");
	LittlevGL_Init();
	createButton("Joshua", 10, 20, 100, 50);
	createButton("Arjun", 120, 20, 100, 50);
	createButton("Sihyung", 10, 90, 100, 50);
	createButton("Paulina", 120, 90, 100, 50);
	
	while(1){
		if(!isDisplayed) {
			lv_task_handler();
			isDisplayed = 1;
		}
  //test display and number parser (can safely be skipped)
	#if DEBUGPRINTS
		//i++;
		// Send Code
		if (i == 1) {
			dateTime.seconds = 0x00;
			dateTime.minutes = 0x59; 
			dateTime.hours = 0x23;
			dateTime.date = 0;
			dateTime.day_int = 0;
			dateTime.month = 0;
			dateTime.year = 0;
		
			stat = setTimeAndDate(&dateTime);		// Send initial time
			UART_OutString("Send Stat: ");
			UART_OutUDec(stat);
			UART_OutString(" ");
			UART_OutString("Ack Ct: ");
			UART_OutUDec(ack_ct);
			UART_OutString("     ");
		}
		
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
