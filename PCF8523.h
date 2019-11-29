#include <stdint.h>
// PCF8523.h
// Runs on TM4C123
// Provide a function that initializes, sends, and receives date and time from PCF8523 RTC module
// Arjun Ramesh and Joshua Iwe
// November 28, 2019

// SCL and SDA lines pulled to +3.3 V with 10 k resistors (part of breakout module)

 
/* TIME VARIABLES */
typedef struct DateTime_t {
	uint8_t seconds;
	uint8_t minutes;
	uint8_t hours;
	uint8_t date;
	uint8_t day_int;
	char* day;
	uint8_t month;
	uint8_t year;
} DateTime;

// I2C0SCL connected to PB2 and to pin 4 of HMC6352 compass or pin 3 of TMP102 thermometer
// I2C0SDA connected to PB3 and to pin 3 of HMC6352 compass or pin 2 of TMP102 thermometer
// SCL and SDA lines pulled to +3.3 V with 10 k resistors (part of breakout module)
// ADD0 pin of TMP102 thermometer connected to GND

/* Initialize I2C0 */
/** PCF8523_I2C0_Init function
* @brief Initializes I2C0 for communication with RTC module
* @details Pins Used
*	         PB2: SCL
*          PB3: SDA
* @params none
*/
void PCF8523_I2C0_Init(void);

/** bin2bcd Function
* @brief	   Converts binary number into BCD format
* @details	 No pins used
*	@param[in] val the number to be converted
*	@param[in] arr pointer to string representation of number
*/
void bin2bcd(int val, char* arr);

/** setTimeAndDate Function 
* @brief 		 Sets the date and time with provided values  
* @details 	 Uses I2C to set time of RTC  
* @param[in] dateTime Object used to hold date and time information
* 
*/
void setTimeAndDate(DateTime* dateTime);
	
/** getTimeAndDate Function 
* @brief 		 Get the date and time from RTC into object 
* @details 	 Uses I2C to get time of RTC and stores it in DateTime object
* @param[in] dateTime Object used to hold date and time information
* 
*/
void getTimeAndDate(DateTime* dateTime);

uint8_t I2C_RTC_Send(int8_t slave, uint8_t register_addr, uint8_t data2);
uint8_t I2C_RTC_Recv(int8_t slave, uint8_t register_base_addr);

/* Send RTC 'num_bytes' bytes from base addr 'register_addr' */
uint8_t I2C_RTC_Send_List(int8_t slave, uint8_t register_addr, uint8_t* sendArr, uint8_t num_bytes);

/* Receive RTC 'num_bytes' bytes from base addr 'register_addr' */
uint8_t I2C_RTC_Recv_List(int8_t slave, uint8_t register_base_addr, uint8_t* receiveArr, uint8_t num_bytes);


