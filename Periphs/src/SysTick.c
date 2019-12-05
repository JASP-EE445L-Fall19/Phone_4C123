//  SysTick.c
//
//
//
//


#include "../inc/SysTick.h"



/**     SysTick_Init Function
 *  @brief      Initializes SysTick. Enabled for periodic interrupts
 *  @param[in]  delay			Period for timer to interrupt 
 * 
 */
void SysTick_Init(uint32_t delay) {
	NVIC_ST_CTRL_R = 0;
	NVIC_ST_RELOAD_R = delay - 1;
	NVIC_ST_CURRENT_R = 0;
	NVIC_ST_CTRL_R = 0x05;
}


/**     SysTick_Wait Function
 *  @brief      Simple delay function using SysTick. 
 *	@details		Counts down every 8.33ns
 *  @param[in]  delay			Period to countdown
 * 
 */
void SysTick_Wait(uint32_t delay) {
  NVIC_ST_RELOAD_R = delay-1;  // number of counts to wait
  NVIC_ST_CURRENT_R = 0;       // any value written to CURRENT clears
  while((NVIC_ST_CTRL_R&0x00010000)==0){} // wait for count flag
}


/**     SysTick_Wait1ms Function
 *  @brief      Delays for 1ms
 *	@details		Counts down every 8.33ns
 *  @param[in]  ms			Number of ms to delay
 * 
 */
void SysTick_Wait1ms(uint32_t ms) {
	for(int i = 0; i < ms; i++){
    SysTick_Wait(80000);  // wait 1ms (assumes 120 MHz clock)
  }
}


/**     SysTick_Wait10ms Function
 *  @brief      Delays for 10ms
 *	@details		Counts down every 8.33ns
 *  @param[in]  ms			Number of 10ms to delay
 * 
 */
void SysTick_Wait10ms(uint32_t ms) {
	for(int i = 0; i < ms; i++) {
		SysTick_Wait(80000);	// wait 10ms		
	}    
}


