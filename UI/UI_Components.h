#include <stdint.h>
#include "../../inc/tm4c123gh6pm.h"
#include "../lvgl/lvgl.h"
#include "../Periphs/inc/ILI9341.h"
#include "../lvgl/lvgl.h"

#define INC_TIME 5

/* LITTLE VGL STUFF */	
void LittlevGL_Init(void);

lv_obj_t* createTime(char* text, int x, int y, int w, int h);

lv_obj_t* createSlider(void);

lv_obj_t* createCallIcon(const lv_img_dsc_t* img);
	
lv_obj_t* createTextIcon(const lv_img_dsc_t* img);

lv_obj_t* createMainText(char* text);

lv_obj_t* createPhoneTextArea(void);

lv_obj_t* createTextMessageArea(void);

lv_obj_t* createPhoneLabel(char* label);

lv_obj_t* createTextLabel(char* label);
