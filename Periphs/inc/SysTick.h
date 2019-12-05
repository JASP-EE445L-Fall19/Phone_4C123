//  SysTick.h
//
//
//
//


#include <stdint.h>
#include "../../../inc/tm4c123gh6pm.h"



/**     SysTick_Init Function
 *  @brief      Initializes SysTick. Enabled for periodic interrupts
 *  @param[in]  delay			Period for timer to interrupt 
 * 
 */
void SysTick_Init(uint32_t delay);



/**     SysTick_Wait Function
 *  @brief      Simple delay function using SysTick. 
 *	@details		Counts down every 8.33ns
 *  @param[in]  delay			Period to countdown
 * 
 */
void SysTick_Wait(uint32_t delay);



/**     SysTick_Wait1ms Function
 *  @brief      Delays for 1ms
 *	@details		Counts down every 8.33ns
 *  @param[in]  ms			Number of ms to delay
 * 
 */
void SysTick_Wait1ms(uint32_t ms);



/**     SysTick_Wait10ms Function
 *  @brief      Delays for 10ms
 *	@details		Counts down every 8.33ns
 *  @param[in]  ms			Number of 10ms to delay
 * 
 */
void SysTick_Wait10ms(uint32_t ms);


