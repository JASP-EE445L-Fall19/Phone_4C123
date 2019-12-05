//  SIM800H.h
//      Header for GSM Module drivers for Lab 11 in EE 445L
//      GSM Module uses the SIM800H chip from SimCom
//
//  Hardware:
//      
//
//  Created: 11/04/2019 - 06:35PM
//  Last Updated: 10/22/2019 - 06:35PM
//
//  Developers:
//      Joshua Iwe
//      Paulina Vasquez-Pena
//      Arjun Ramesh
//      Sihyung Woo
//


#include <stdint.h>
#include <stdio.h>
#include "../../Periphs/inc/UART.h"
#include "../../Periphs/inc/SysTick.h"
#include "../../Periphs/inc/Fifo_Custom.h"




/**     SIM800H_Init Function
 *  @brief      Initializes UART2 and GSM module
 *  @details    Pins Used:
 *                  PD0 - Rst
 *                  PD1 - PS
 *                  PD2 - Key
 *                  PD3 - RI
 *                  PD4 - UART2_Rx
 *                  PD5 - UART2_Tx
 *                  PD7 - NS
 *  @note       Might have the baudrate as the input parameter
 *                  but the module automatically tunes baudrate when
 *                  you send first commands to it.
 */
void SIM800H_Init(void);


/**     SIM800H_SimCardNumber Function
 *  @brief      Retrieves Sim Card number of currently installed card
 *							Displays to terminal using printf
 *
 */
void SIM800H_SimCardNumber(void);


/**     SIM800H_CheckSignalStrength Function
 *  @brief      Gets signal strength. First number represented in dB, greater than 5 is good
 *							Displays to terminal using printf
 *
 */
void SIM800H_CheckSignalStrength(void);


/**     SIM800H_CheckBattery Function
 *  @brief      Gets battery level. Second number is percentage left, third is voltage level in mV
 *							Displays to terminal using printf
 *
 */
void SIM800H_CheckBattery(void);


/**     SIM800H_SendText Function
 *  @brief      Sends a text to specified number
 *							Displays to terminal using printf
 *	@param[in]	phone[]			Null-terminated char array of phone number (total 10 digits + null)
 *	@param[in]	message[]		Null-terminated char array of message
 */
void SIM800H_SendText(char phone[], char message[]);



/**     SIM800H_EnableBuzzer Function
 *  @brief      Enables PWM for vibration motor. Activates when you receive notification
 *
 */
void SIM800H_EnableBuzzer(void);


/**     SIM800H_PickUpPhone Function
 *  @brief      When received a call, uses function to pick up
 *
 */
void SIM800H_PickUpPhone(void);


/**     SIM800H_HangUpPhone Function
 *  @brief      Hangs up phone while during a call
 *
 */
void SIM800H_HangUpPhone(void);


/**     SIM800H_CallPhone Function
 *  @brief      Calls the specified phone number.
 *	@param[in]	nummber[]			Null-terminated char array of number (10 digits + NULL);
 *
 */
void SIM800H_CallPhone(char number[]) ;



