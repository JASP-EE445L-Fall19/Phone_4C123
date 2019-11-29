// PCF8523.c
// Runs on TM4C123
// Provide a function that initializes, sends, and receives date and time from PCF8523 RTC module
// Arjun Ramesh and Joshua Iwe
// November 28, 2019

// SCL and SDA lines pulled to +3.3 V with 10 k resistors (part of breakout module)

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "PCF8523.h"

#define I2C_MCS_ACK             0x00000008  // Data Acknowledge Enable
#define I2C_MCS_DATACK          0x00000008  // Acknowledge Data
#define I2C_MCS_ADRACK          0x00000004  // Acknowledge Address
#define I2C_MCS_STOP            0x00000004  // Generate STOP
#define I2C_MCS_START           0x00000002  // Generate START
#define I2C_MCS_ERROR           0x00000002  // Error
#define I2C_MCS_RUN             0x00000001  // I2C Master Enable
#define I2C_MCS_BUSY            0x00000001  // I2C Busy
#define I2C_MCR_MFE             0x00000010  // I2C Master Function Enable

#define RTC_ADDR 		0x68			// slave addr for PCF
#define TIME_BASE		0x03			// Base addr

#define MAXRETRIES              5           // number of receive attempts before giving up

int ack_ct = 0;
uint8_t err_code = 0;

char day_string[7][5] = {"Sun", "Mon", "Tues", "Wed", "Thur", "Fri", "Sat"};	

void PCF8523_I2C0_Init(void){
  SYSCTL_RCGCI2C_R |= 0x0001;           // activate I2C0
  SYSCTL_RCGCGPIO_R |= 0x0002;          // activate port B
  while((SYSCTL_PRGPIO_R&0x0002) == 0){};// ready?

  GPIO_PORTB_AFSEL_R |= 0x0C;           // 3) enable alt funct on PB2,3
  GPIO_PORTB_ODR_R |= 0x08;             // 4) enable open drain on PB3 only
  GPIO_PORTB_DEN_R |= 0x0C;             // 5) enable digital I/O on PB2,3
                                        // 6) configure PB2,3 as I2C
  GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R&0xFFFF00FF)+0x00003300;
  GPIO_PORTB_AMSEL_R &= ~0x0C;          // 7) disable analog functionality on PB2,3
  I2C0_MCR_R = I2C_MCR_MFE;      // 9) master function enable
  I2C0_MTPR_R = 24;              // 8) configure for 100 kbps clock
  // 20*(TPR+1)*20ns = 10us, with TPR=24
}

uint8_t checkError() {
	if((I2C0_MCS_R&(I2C_MCS_DATACK|I2C_MCS_ADRACK|I2C_MCS_ERROR)) != 0){
    I2C0_MCS_R = (0                // send stop if nonzero
                     //  & ~I2C_MCS_ACK     // no data ack (no data on send)
                       | I2C_MCS_STOP     // stop
                     //  & ~I2C_MCS_START   // no start/restart
                     //  & ~I2C_MCS_RUN    // master disable
                        );   
                                          // return error bits if nonzero
    return (I2C0_MCS_R&(I2C_MCS_DATACK|I2C_MCS_ADRACK|I2C_MCS_ERROR));
  }
	return 0;
}


uint8_t I2C_RTC_Send(int8_t slave, uint8_t register_addr, uint8_t data2) {
  ack_ct = 0;
	/* Send slave address, register address */
  while(I2C0_MCS_R&I2C_MCS_BUSY){};// wait for I2C ready
  I2C0_MSA_R = (slave<<1)&0xFE;    // MSA[7:1] is slave address
  I2C0_MSA_R &= ~0x01;             // MSA[0] is 0 for send
  I2C0_MDR_R = register_addr&0xFF;         // prepare first byte
  I2C0_MCS_R = (0
                     //  & ~I2C_3MCS_ACK     // no data ack (no data on send)
                    //   & ~I2C_MCS_STOP    // no stop
                       | I2C_MCS_START    // generate start/restart
                       | I2C_MCS_RUN);    // master enable
  while(I2C0_MCS_R&I2C_MCS_BUSY){};// wait for transmission done
                                          // check error bits
  if ((err_code = checkError()))
		return err_code;

	ack_ct = 1;
	/* Send Data Bytes */
  I2C0_MDR_R = data2&0xFF;         // prepare second byte
  I2C0_MCS_R = (0
                      // & ~I2C_MCS_ACK     // no data ack (no data on send)
                       | I2C_MCS_STOP     // generate stop
                      // & ~I2C_MCS_START   // no start/restart
                       | I2C_MCS_RUN);    // master enable
  while(I2C0_MCS_R&I2C_MCS_BUSY){};// wait for transmission done
                                          // return error bits
  ack_ct = 2;
	return (I2C0_MCS_R&(I2C_MCS_DATACK|I2C_MCS_ADRACK|I2C_MCS_ERROR));
}

uint8_t I2C_RTC_Send_List(int8_t slave, uint8_t register_addr, uint8_t* sendArr, uint8_t num_bytes) {
  ack_ct = 0;
	/* Send slave address, register address */
  while(I2C0_MCS_R&I2C_MCS_BUSY){};// wait for I2C ready
  I2C0_MSA_R = (slave<<1)&0xFE;    // MSA[7:1] is slave address
  I2C0_MSA_R &= ~0x01;             // MSA[0] is 0 for send
  I2C0_MDR_R = register_addr&0xFF;         // prepare first byte
  I2C0_MCS_R = (0
                     //  & ~I2C_3MCS_ACK     // no data ack (no data on send)
                    //   & ~I2C_MCS_STOP    // no stop
                       | I2C_MCS_START    // generate start/restart
                       | I2C_MCS_RUN);    // master enable
  while(I2C0_MCS_R&I2C_MCS_BUSY){};// wait for transmission done
                                          // check error bits
  if ((err_code = checkError()))
		return err_code;

	ack_ct++;
	
	/* Send Data Bytes */
	uint8_t ct = 0;
  for (int i = 0; i < num_bytes; i++) {
		I2C0_MDR_R = (sendArr[ct++]&0xFF);         // prepare byte
		I2C0_MCS_R = (0 | I2C_MCS_RUN | (I2C_MCS_STOP * (i == num_bytes - 1)));    // master enable and stop on last byte

		while(I2C0_MCS_R&I2C_MCS_BUSY){};					 // wait for transmission done
		if ((err_code = checkError()))
			return err_code;
		ack_ct++;
	}
	return (I2C0_MCS_R&(I2C_MCS_DATACK|I2C_MCS_ADRACK|I2C_MCS_ERROR));
}


uint8_t I2C_RTC_Recv_List(int8_t slave, uint8_t register_base_addr, uint8_t* receiveArr, uint8_t num_bytes) { 
	ack_ct = 0;
	/* Send slave address, register address */
  while(I2C0_MCS_R&I2C_MCS_BUSY){};// wait for I2C ready
  I2C0_MSA_R = (slave<<1)&0xFE;    // MSA[7:1] is slave address
  I2C0_MSA_R &= ~0x01;             // MSA[0] is 0 for send
  I2C0_MDR_R = register_base_addr&0xFF;         // prepare first byte
  I2C0_MCS_R = (0
                     //  & ~I2C_3MCS_ACK     // no data ack (no data on send)
                       | I2C_MCS_STOP    // stop
                       | I2C_MCS_START    // generate start/restart
                       | I2C_MCS_RUN);    // master enable
  while(I2C0_MCS_R&I2C_MCS_BUSY){};// wait for transmission done
                                          // check error bits
  if ((err_code = checkError()))
		return err_code;
	
	ack_ct++;
	
	/* Receive Data Bytes */
	I2C0_MSA_R = (slave<<1)&0xFE;    // MSA[7:1] is slave address
	I2C0_MSA_R |= 0x01;             // MSA[0] is 1 for read		
	uint8_t ct = 0;
	uint8_t ctl_reg = 0; 
	
	for(int i = 0; i < num_bytes; i++) {
		
		ctl_reg = (I2C_MCS_RUN | I2C_MCS_ACK);					// Always master enable and ack
		if (num_bytes == 1)															// If only one byte, stop
			ctl_reg |= (I2C_MCS_START | I2C_MCS_STOP);
		else if (i == 0) {															// First byte of many
			ctl_reg |= (I2C_MCS_START);
		}
		else if (i == num_bytes - 1) {									// Last byte
			ctl_reg |= (I2C_MCS_STOP);    					
		}
		
		I2C0_MCS_R = (0 | ctl_reg);									// set controls for transmission
		
		while(I2C0_MCS_R&I2C_MCS_BUSY){};// wait for transmission done
    receiveArr[ct++] = (I2C0_MDR_R&0xFF);	
		if ((err_code = checkError()))
			return err_code;
		ack_ct++;
	}
	return 0;
}

uint8_t I2C_RTC_Recv(int8_t slave, uint8_t register_base_addr) {
	 ack_ct = 0;
	/* Send slave address, register address */
  while(I2C0_MCS_R&I2C_MCS_BUSY){};// wait for I2C ready
  I2C0_MSA_R = (slave<<1)&0xFE;    // MSA[7:1] is slave address
  I2C0_MSA_R &= ~0x01;             // MSA[0] is 0 for send
  I2C0_MDR_R = register_base_addr&0xFF;         // prepare first byte
  I2C0_MCS_R = (0
                     //  & ~I2C_3MCS_ACK     // no data ack (no data on send)
                       | I2C_MCS_STOP    // stop
                       | I2C_MCS_START    // generate start/restart
                       | I2C_MCS_RUN);    // master enable
  while(I2C0_MCS_R&I2C_MCS_BUSY){};// wait for transmission done
                                          // check error bits
  if ((err_code = checkError()))
		return err_code;
	
	ack_ct++;
	/* Receive Data Bytes */
  I2C0_MSA_R = (slave<<1)&0xFE;    // MSA[7:1] is slave address
  I2C0_MSA_R |= 0x01;             // MSA[0] is 1 for read
  I2C0_MCS_R = (0
                       | I2C_MCS_ACK     // no data ack (no data on send)
                       | I2C_MCS_STOP     // generate stop
                       | I2C_MCS_START   // no start/restart
                       | I2C_MCS_RUN);    // master enable
  while(I2C0_MCS_R&I2C_MCS_BUSY){};// wait for transmission done
                                          // return error bits
  ack_ct++;
	return (I2C0_MDR_R&0xFF);
}

void bin2bcd(int val, char* arr) {
	arr[0] = ((val >> 4) & 0x07) + '0';
	arr[1] = (val & 0x0F) + '0';
	arr[2] = 0;
}

void getTimeAndDate(DateTime* dateTime) {
	uint8_t RTC_Buf[7];
	int status = I2C_RTC_Recv_List(RTC_ADDR, TIME_BASE, RTC_Buf, 7);
	
	/* Assign globals */
	dateTime->seconds = RTC_Buf[0];
	dateTime->minutes = RTC_Buf[1];
	dateTime->hours = RTC_Buf[2];
	dateTime->date = RTC_Buf[3];
	dateTime->day_int = RTC_Buf[4];
	dateTime->day = day_string[dateTime->day_int];
	dateTime->month = RTC_Buf[5];
	dateTime->year = RTC_Buf[6];

}
	
void setTimeAndDate(DateTime* dateTime) {
	uint8_t time_info[7] = {dateTime->seconds, 
													dateTime->minutes,
													dateTime->hours,
													dateTime->date,
													dateTime->day_int,
													dateTime->month,
													dateTime->year
	};
	int status = I2C_RTC_Send_List(RTC_ADDR, TIME_BASE, time_info, 7);										
}
