#include <stdint.h>
#include "../../inc/tm4c123gh6pm.h"
#include "../Periphs/inc/ILI9341.h"
#include "UI_Components.h"

/* LITTLE VGL STUFF */	
void LvGL_Timer0_Init(int32_t period){
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
	LvGL_Timer0_Init(INC_TIME * 80000);
	lv_init();
	/* Set default theme */
	lv_theme_t * th = lv_theme_night_init(20, NULL);
	lv_theme_set_current(th);
	//lv_style_scr.body.main_color = LV_COLOR_RED;
	//lv_style_scr.body.grad_color = LV_COLOR_RED;
	
	lv_disp_buf_init(&disp_buf, buf, NULL, LV_HOR_RES_MAX * 20);    /*Initialize the display buffer*/
	lv_disp_drv_init(&disp_drv);          /*Basic initialization*/
	disp_drv.flush_cb = my_disp_flush;    /*Set your driver function*/
	disp_drv.buffer = &disp_buf;          /*Assign the buffer to the display*/
	lv_disp_drv_register(&disp_drv);      /*Finally register the driver*/
}

lv_obj_t* time_label;
//* Create a Button with text and size specs*/
lv_obj_t* createTime(char* text, int x, int y, int w, int h) {
	lv_obj_t* btn = lv_btn_create(lv_scr_act(), NULL);     /*Add a button the current screen*/
	lv_obj_set_pos(btn, x, y);                            /*Set its position*/
	lv_obj_set_size(btn, w, h);                          /*Set its size*/
	time_label = lv_label_create(btn, NULL);          /*Add a label to the button*/
	lv_label_set_text(time_label, text);                     /*Set the labels text*/
	lv_label_set_align(time_label, LV_LABEL_ALIGN_CENTER);
	return btn;
}

/* Create a slider with text and size specs*/
lv_obj_t* createSlider() {
	lv_obj_t * slider = lv_slider_create(lv_scr_act(), NULL);
	lv_obj_set_width(slider, 40);                        /*Set the width*/
	lv_obj_align(slider, NULL, LV_ALIGN_CENTER, 0, 0);    /*Align to the center of the parent (screen)*/
	return slider;
}


lv_obj_t* createCallIcon(const lv_img_dsc_t* img) {
	lv_obj_t* btn = lv_btn_create(lv_scr_act(), NULL);     /*Add a button the current screen*/
	lv_obj_set_size(btn, 100, 120);                          /*Set its size*/
	lv_obj_align(btn, NULL, LV_ALIGN_IN_BOTTOM_LEFT, 10, -10);
	
	lv_obj_t * bitmap = lv_img_create(btn, NULL);
  lv_img_set_src(bitmap, img);
  lv_obj_align(bitmap, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, 0);
	
	lv_obj_t* call_label = lv_label_create(btn, NULL);          /*Add a label to the button*/
	lv_label_set_text(call_label, "Call (*)");                     /*Set the labels text*/
	lv_obj_align(call_label, NULL, LV_ALIGN_IN_TOP_MID, 0, 0);
	
	return bitmap;
}


lv_obj_t* createTextIcon(const lv_img_dsc_t* img) {
	lv_obj_t* btn = lv_btn_create(lv_scr_act(), NULL);     /*Add a button the current screen*/
	lv_obj_set_size(btn, 100, 120);                          /*Set its size*/
	lv_obj_align(btn, NULL, LV_ALIGN_IN_BOTTOM_RIGHT, -10, -10);
	
	lv_obj_t * bitmap = lv_img_create(btn, NULL);
  lv_img_set_src(bitmap, img);
  lv_obj_align(bitmap, NULL, LV_ALIGN_CENTER, 0, 0);
	
	lv_obj_t* text_label = lv_label_create(btn, NULL);          /*Add a label to the button*/
	lv_label_set_text(text_label, "Text (#)");                     /*Set the labels text*/
	lv_obj_align(text_label, NULL, LV_ALIGN_IN_TOP_MID, 0, 0);
	
	return bitmap;
}


lv_obj_t* createMainText(char* text) {
	lv_obj_t* main_label = lv_label_create(lv_scr_act(), NULL);
	lv_label_set_text(main_label, text);                     /*Set the labels text*/
	lv_obj_align(main_label, NULL, LV_ALIGN_CENTER, 0, -30);
	lv_label_set_long_mode(main_label, LV_LABEL_LONG_SROLL_CIRC);	
	return main_label;
}