//  SIM800H.c
//      Source code for GSM Module drivers for Lab 11 in EE 445L
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


#include "../inc/SIM800H.h"


int checkForOK(char buf[], int size);
void WaitForOK(void);
void TerminalMenu(void);


/**     SIM800H_Init Function
 *  @brief      Initializes UART5 and GSM module
 *  @details    Pins Used:
 *                  PD0 - Rst
 *                  PD1 - PS
 *                  PD2 - Key
 *                  PD3 - RI
 *                  PD4 - UART5_Rx
 *                  PD5 - UART5_Tx
 *                  PD7 - NS
 *  @note       Might have the baudrate as the input parameter
 *                  but the module automatically tunes baudrate when
 *                  you send first commands to it.
 *							Based on Adafruit FONA library
 */
void SIM800H_Init(void) {
	char dump;
	
	UART_Init(115200);
	SysTick_Init(25*80000);
	// Initialize other config pins
	SYSCTL_RCGCGPIO_R |= 0x08;			// Port D
	while ((SYSCTL_PRGPIO_R & 0x08) == 0) {};
	GPIO_PORTD_DIR_R |= 0x04;				// RST pin
	GPIO_PORTD_DEN_R |= 0x04;
	
	// Reset Module (pull rst pin 100ms)
	GPIO_PORTD_DATA_R |= 0x04;
	SysTick_Wait10ms(1);		// Wait 10ms
	GPIO_PORTD_DATA_R &= ~0x04;
	SysTick_Wait10ms(10);		// Wait 100ms
	GPIO_PORTD_DATA_R |= 0x04;
		

	// See if SIM800H is responding correctly (timeout if not)	
	while(Fifo_Get(&dump));	
	UART_OutString("AT\r");
	SysTick_Wait10ms(5);
	WaitForOK();
	
//	// Display name and revision.			Never receives OK so I check the latest revision number (8)
//	while(Fifo_Get(&dump));
//	UART_OutString("ATI\r");
//	SysTick_Wait10ms(5);
//	while(Fifo_Get(&dump)) {
//		if(dump != 0x0A);	//printf("%c", dump);
//		//SysTick_Wait10ms(10);
//	}
//	dump = UART_InChar();			// Doesn't interrupt even though OK is in UART FIFO
//	dump = UART_InChar();			// Gotta d it manually
//	//printf("OK\r");
//	while(Fifo_Get(&dump));

//	SIM800H_SimCardNumber();
//	
	// Check Network
//	UART2_OutString("AT+COPS?\r");
//	//printf("AT+COPS?\r");
//	SysTick_Wait10ms(10);
//	WaitForOK();
	
	//SIM800H_CheckSignalStrength();
	//SIM800H_CheckBattery();
	
	//TerminalMenu();							// Comment in or out for terminal menu	
	
}


/**     SIM800H_SimCardNumber Function
 *  @brief      Retrieves Sim Card number of currently installed card
 *							Displays to terminal using printf
 *
 */
void SIM800H_SimCardNumber(void) {
	// Display SIM Card ID
	UART_OutString("AT+CCID\r");
	SysTick_Wait10ms(5);
	WaitForOK();	
	
}




/**     SIM800H_CheckSignalStrength Function
 *  @brief      Gets signal strength. First number represented in dB, greater than 5 is good
 *							Displays to terminal using printf
 *
 */
void SIM800H_CheckSignalStrength(void) {
	// Check Signal Strength (dB - Higher, better)
	UART_OutString("AT+CSQ\r");
	//printf("AT+CSQ\r");
	SysTick_Wait10ms(5);
	WaitForOK();
	
}



/**     SIM800H_CheckBattery Function
 *  @brief      Gets battery level. Second number is percentage left, third is voltage level in mV
 *							Displays to terminal using printf
 *
 */
void SIM800H_CheckBattery(void) {
	// Check Battery Life (second number percentage, third number battery voltage in mV);
	UART_OutString("AT+CBC\r");
	//printf("AT+CBC\r");
	SysTick_Wait10ms(5);
	WaitForOK();
	
}


/**     SIM800H_SendText Function
 *  @brief      Sends a text to specified number
 *							Displays to terminal using printf
 *	@param[in]	phone[]			Null-terminated char array of phone number (total 10 digits + null)
 *	@param[in]	message[]		Null-terminated char array of message
 */
void SIM800H_SendText(char phone[], char message[]) {
	UART_OutString("AT+CMGF=1\r");
	SysTick_Wait10ms(5);
	WaitForOK();
	
	char number[14] = {'\"','2','5','4','7','6','0','9','5','9','2','\"','\r',0};
	UART_OutString("AT+CMGS=");
	UART_OutString(number);
	//printf("-----   AT+CMGS=%s", number);
	SysTick_Wait10ms(50);
	UART_OutString("message");
	SysTick_Wait10ms(2);
	UART_OutChar('\r');
	SysTick_Wait10ms(2);
	char ctrlZ[3] = {0x1A, '\r', 0}; 
	UART_OutChar(0x1A);
	SysTick_Wait10ms(2);
	UART_OutChar('\r');
	
	WaitForOK();
	
	
}



void SIM800H_ReadText(void) {
	
	
	
}




/**     SIM800H_EnableBuzzer Function
 *  @brief      Enables PWM for vibration motor. Activates when you receive notification
 *
 */
void SIM800H_EnableBuzzer(void) {
	UART_OutString("AT+SPWM=0,10000,5000\r");
	SysTick_Wait10ms(5);
	WaitForOK();	
	
}


/**     SIM800H_PickUpPhone Function
 *  @brief      When received a call, uses function to pick up
 *
 */
void SIM800H_PickUpPhone(void) {
	UART_OutString("ATA\r");
	SysTick_Wait1ms(5);
	
	char OChar = 0;
	char KChar = 0;
	char dump;
	while((OChar != 'O' && KChar != 'K')) {
		Fifo_Get(&dump);
		if(dump != 0x0A); //printf("%c", dump);
		if(dump == 'O') {
			OChar = dump;
			Fifo_Get(&dump);
			//printf("%c", dump);
			if(dump == 'K') {
				KChar = dump;
			} else {
				OChar = 0;
			}
		} else if(dump == 'N') {
			char noCarrier[10] = {'N', 'O', ' ', 'C', 'A', 'R', 'R', 'I', 'E', 'R'};
			int noCarrierFlag = 0;
			for(int i = 0; i < 10; i++) {
				if(dump == noCarrier[i]) {
					Fifo_Get(&dump);
					noCarrierFlag = 1;
				} else {
					noCarrierFlag = 0;
					break;
				}
			}
			
			if(noCarrierFlag == 1);// printf("No Carrier\r");
		}
	}
	//printf("\r");
	while(Fifo_Get(&dump));	
	
	
}



/**     SIM800H_HangUpPhone Function
 *  @brief      Hangs up phone while during a call
 *
 */
void SIM800H_HangUpPhone(void) {
	UART_OutString("ATH\r");
	SysTick_Wait1ms(5);
	WaitForOK();	
	
}

/**     SIM800H_CallPhone Function
 *  @brief      Calls the specified phone number.
 *	@param[in]	nummber[]			Null-terminated char array of number (10 digits + NULL);
 *
 */
void SIM800H_CallPhone(char number[]) {
	UART_OutString("ATD");
	UART_OutString(number);
	UART_OutChar(';');
	UART_OutChar('\r');
	SysTick_Wait1ms(5);
	
	WaitForOK();	
	
}






int checkForOK(char buf[], int size) {
	for(int i = 0; i < size-1; i++) {
		if(buf[i] == 'O' && buf[i+1] == 'K') {
			return 1;
		}
	}
	return 0;
	
}


void WaitForOK(void) {
	char OChar = 0;
	char KChar = 0;
	char dump;
	while((OChar != 'O' && KChar != 'K')) {
		Fifo_Get(&dump);
		if(dump != 0x0A); //printf("%c", dump);
		if(dump == 'O') {
			OChar = dump;
			Fifo_Get(&dump);
			//printf("%c", dump);
			if(dump == 'K') {
				KChar = dump;
			} else {
				OChar = 0;
			}
		}
		SysTick_Wait10ms(10);
	}
	//printf("\r");
	while(Fifo_Get(&dump));
}



void TerminalMenu(void) {
	//printf("[?] Print this menu\r");
	//printf("[a] read the ADC 2.8V max (FONA800 & 808)\r");
	
}








