/***************************************************
  This is a library for the Adafruit 1.8" SPI display.
  This library works with the Adafruit 1.8" TFT Breakout w/SD card
  ----> http://www.adafruit.com/products/358
  as well as Adafruit raw 1.8" TFT display
  ----> http://www.adafruit.com/products/618

  Check out the links above for our tutorials and wiring diagrams
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional)
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/

// ILI9341.h
// Runs on LM4F120/TM4C123
// Low level drivers for the ILI9341 160x128 LCD based off of
// the file described above.
//    16-bit color, 128 wide by 160 high LCD
// Daniel Valvano, March 30, 2015
// Augmented 7/17/2014 to have a simple graphics facility
// Tested with LaunchPadDLL.dll simulator 9/2/2014

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to ARM Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2014

 Copyright 2015 by Jonathan W. Valvano, valvano@mail.utexas.edu
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

// hardware connections
// **********ILI9341 TFT and SDC*******************
// ILI9341
// Backlight (pin 10) connected to +3.3 V
// MISO (pin 9) unconnected
// SCK (pin 8) connected to PA2 (SSI0Clk)
// MOSI (pin 7) connected to PA5 (SSI0Tx)
// TFT_CS (pin 6) connected to PA3 (SSI0Fss)
// CARD_CS (pin 5) unconnected
// Data/Command (pin 4) connected to PA6 (GPIO), high for data, low for command
// RESET (pin 3) connected to PA7 (GPIO)
// VCC (pin 2) connected to +3.3 V
// Gnd (pin 1) connected to ground

// **********wide.hk ILI9341R with ADXL345 accelerometer *******************
// Silkscreen Label (SDC side up; LCD side down) - Connection
// VCC  - +3.3 V
// GND  - Ground
// !SCL - PA2 Sclk SPI clock from microcontroller to TFT or SDC
// !SDA - PA5 MOSI SPI data from microcontroller to TFT or SDC
// DC   - PA6 TFT data/command
// RES  - PA7 TFT reset
// CS   - PA3 TFT_CS, active low to enable TFT
// *CS  - (NC) SDC_CS, active low to enable SDC
// MISO - (NC) MISO SPI data from SDC to microcontroller
// SDA  – (NC) I2C data for ADXL345 accelerometer
// SCL  – (NC) I2C clock for ADXL345 accelerometer
// SDO  – (NC) I2C alternate address for ADXL345 accelerometer
// Backlight + - Light, backlight connected to +3.3 V

// **********wide.hk ILI9341R with ADXL335 accelerometer *******************
// Silkscreen Label (SDC side up; LCD side down) - Connection
// VCC  - +3.3 V
// GND  - Ground
// !SCL - PA2 Sclk SPI clock from microcontroller to TFT or SDC
// !SDA - PA5 MOSI SPI data from microcontroller to TFT or SDC
// DC   - PA6 TFT data/command
// RES  - PA7 TFT reset
// CS   - PA3 TFT_CS, active low to enable TFT
// *CS  - (NC) SDC_CS, active low to enable SDC
// MISO - (NC) MISO SPI data from SDC to microcontroller
// X– (NC) analog input X-axis from ADXL335 accelerometer
// Y– (NC) analog input Y-axis from ADXL335 accelerometer
// Z– (NC) analog input Z-axis from ADXL335 accelerometer
// Backlight + - Light, backlight connected to +3.3 V
#include<stdint.h>

#ifndef _ILI9341H_
#define _ILI9341H_

// some flags for ILI9341_InitR()
enum initRFlags{
  none,
  INITR_GREENTAB,
  INITR_REDTAB,
  INITR_BLACKTAB
};

#define ILI9341_TFTWIDTH  240
#define ILI9341_TFTHEIGHT 320


// Color definitions
#define ILI9341_BLACK   0x0000
#define ILI9341_BLUE    0xF800
#define ILI9341_RED     0x001F
#define ILI9341_GREEN   0x07E0
#define ILI9341_CYAN    0xFFE0
#define ILI9341_MAGENTA 0xF81F
#define ILI9341_YELLOW  0x07FF
#define ILI9341_WHITE   0xFFFF

//------------ILI9341_InitB------------
// Initialization for ILI9341B screens.
// Input: none
// Output: none
void ILI9341_InitB(void);


//------------ILI9341_InitR------------
// Initialization for ILI9341R screens (green or red tabs).
// Input: option one of the enumerated options depending on tabs
// Output: none
void ILI9341_InitR(enum initRFlags option);


//------------ILI9341_DrawPixel------------
// Color the pixel at the given coordinates with the given color.
// Requires 13 bytes of transmission
// Input: x     horizontal position of the pixel, columns from the left edge
//               must be less than 128
//               0 is on the left, 126 is near the right
//        y     vertical position of the pixel, rows from the top edge
//               must be less than 160
//               159 is near the wires, 0 is the side opposite the wires
//        color 16-bit color, which can be produced by ILI9341_Color565()
// Output: none
void ILI9341_DrawPixel(int16_t x, int16_t y, uint16_t color);

//------------ILI9341_DrawFastVLine------------
// Draw a vertical line at the given coordinates with the given height and color.
// A vertical line is parallel to the longer side of the rectangular display
// Requires (11 + 2*h) bytes of transmission (assuming image fully on screen)
// Input: x     horizontal position of the start of the line, columns from the left edge
//        y     vertical position of the start of the line, rows from the top edge
//        h     vertical height of the line
//        color 16-bit color, which can be produced by ILI9341_Color565()
// Output: none
void ILI9341_DrawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);


//------------ILI9341_DrawFastHLine------------
// Draw a horizontal line at the given coordinates with the given width and color.
// A horizontal line is parallel to the shorter side of the rectangular display
// Requires (11 + 2*w) bytes of transmission (assuming image fully on screen)
// Input: x     horizontal position of the start of the line, columns from the left edge
//        y     vertical position of the start of the line, rows from the top edge
//        w     horizontal width of the line
//        color 16-bit color, which can be produced by ILI9341_Color565()
// Output: none
void ILI9341_DrawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);


//------------ILI9341_FillScreen------------
// Fill the screen with the given color.
// Requires 40,971 bytes of transmission
// Input: color 16-bit color, which can be produced by ILI9341_Color565()
// Output: none
void ILI9341_FillScreen(uint16_t color);


//------------ILI9341_FillRect------------
// Draw a filled rectangle at the given coordinates with the given width, height, and color.
// Requires (11 + 2*w*h) bytes of transmission (assuming image fully on screen)
// Input: x     horizontal position of the top left corner of the rectangle, columns from the left edge
//        y     vertical position of the top left corner of the rectangle, rows from the top edge
//        w     horizontal width of the rectangle
//        h     vertical height of the rectangle
//        color 16-bit color, which can be produced by ILI9341_Color565()
// Output: none
void ILI9341_FillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);


//------------ILI9341_Color565------------
// Pass 8-bit (each) R,G,B and get back 16-bit packed color.
// Input: r red value
//        g green value
//        b blue value
// Output: 16-bit color
uint16_t ILI9341_Color565(uint8_t r, uint8_t g, uint8_t b);

//------------vGL2ILI_Color------------
// Pass R (5-bit) ,G (6-bit) ,B (5-bit) and get back 16-bit packed color. (for interface with LittlevGL)
// Input: r red value
//        g green value
//        b blue value
// Output: 16-bit color
uint16_t vGL2ILI_Color(uint8_t red, uint8_t green, uint8_t blue);

//------------ILI9341_SwapColor------------
// Swaps the red and blue values of the given 16-bit packed color;
// green is unchanged.
// Input: x 16-bit color in format B, G, R
// Output: 16-bit color in format R, G, B
uint16_t ILI9341_SwapColor(uint16_t x) ;


//------------ILI9341_DrawBitmap------------
// Displays a 16-bit color BMP image.  A bitmap file that is created
// by a PC image processing program has a header and may be padded
// with dummy columns so the data have four byte alignment.  This
// function assumes that all of that has been stripped out, and the
// array image[] has one 16-bit halfword for each pixel to be
// displayed on the screen (encoded in reverse order, which is
// standard for bitmap files).  An array can be created in this
// format from a 24-bit-per-pixel .bmp file using the associated
// converter program.
// (x,y) is the screen location of the lower left corner of BMP image
// Requires (11 + 2*w*h) bytes of transmission (assuming image fully on screen)
// Input: x     horizontal position of the bottom left corner of the image, columns from the left edge
//        y     vertical position of the bottom left corner of the image, rows from the top edge
//        image pointer to a 16-bit color BMP image
//        w     number of pixels wide
//        h     number of pixels tall
// Output: none
// Must be less than or equal to 128 pixels wide by 160 pixels high
void ILI9341_DrawBitmap(int16_t x, int16_t y, const uint16_t *image, int16_t w, int16_t h);

//------------ILI9341_DrawCharS------------
// Simple character draw function.  This is the same function from
// Adafruit_GFX.c but adapted for this processor.  However, each call
// to ILI9341_DrawPixel() calls setAddrWindow(), which needs to send
// many extra data and commands.  If the background color is the same
// as the text color, no background will be printed, and text can be
// drawn right over existing images without covering them with a box.
// Requires (11 + 2*size*size)*6*8 (image fully on screen; textcolor != bgColor)
// Input: x         horizontal position of the top left corner of the character, columns from the left edge
//        y         vertical position of the top left corner of the character, rows from the top edge
//        c         character to be printed
//        textColor 16-bit color of the character
//        bgColor   16-bit color of the background
//        size      number of pixels per character pixel (e.g. size==2 prints each pixel of font as 2x2 square)
// Output: none
void ILI9341_DrawCharS(int16_t x, int16_t y, char c, int16_t textColor, int16_t bgColor, uint8_t size);

//------------ILI9341_DrawChar------------
// Advanced character draw function.  This is similar to the function
// from Adafruit_GFX.c but adapted for this processor.  However, this
// function only uses one call to setAddrWindow(), which allows it to
// run at least twice as fast.
// Requires (11 + size*size*6*8) bytes of transmission (assuming image fully on screen)
// Input: x         horizontal position of the top left corner of the character, columns from the left edge
//        y         vertical position of the top left corner of the character, rows from the top edge
//        c         character to be printed
//        textColor 16-bit color of the character
//        bgColor   16-bit color of the background
//        size      number of pixels per character pixel (e.g. size==2 prints each pixel of font as 2x2 square)
// Output: none
void ILI9341_DrawChar(int16_t x, int16_t y, char c, int16_t textColor, int16_t bgColor, uint8_t size);

//------------ILI9341_DrawString------------
// String draw function.
// 16 rows (0 to 15) and 21 characters (0 to 20)
// Requires (11 + size*size*6*8) bytes of transmission for each character
// Input: x         columns from the left edge (0 to 20)
//        y         rows from the top edge (0 to 15)
//        pt        pointer to a null terminated string to be printed
//        textColor 16-bit color of the characters
// bgColor is Black and size is 1
// Output: number of characters printed
uint32_t ILI9341_DrawString(uint16_t x, uint16_t y, char *pt, int16_t textColor);;



//********ILI9341_SetCursor*****************
// Move the cursor to the desired X- and Y-position.  The
// next character will be printed here.  X=0 is the leftmost
// column.  Y=0 is the top row.
// inputs: newX  new X-position of the cursor (0<=newX<=20)
//         newY  new Y-position of the cursor (0<=newY<=15)
// outputs: none
void ILI9341_SetCursor(uint32_t newX, uint32_t newY);

//-----------------------ILI9341_OutUDec-----------------------
// Output a 32-bit number in unsigned decimal format
// Position determined by ILI9341_SetCursor command
// Color set by ILI9341_SetTextColor
// Input: 32-bit number to be transferred
// Output: none
// Variable format 1-10 digits with no space before or after
void ILI9341_OutUDec(uint32_t n);


//------------ILI9341_SetRotation------------
// Change the image rotation.
// Requires 2 bytes of transmission
// Input: m new rotation value (0 to 3)
// Output: none
void ILI9341_SetRotation(uint8_t m) ;


//------------ILI9341_InvertDisplay------------
// Send the command to invert all of the colors.
// Requires 1 byte of transmission
// Input: i 0 to disable inversion; non-zero to enable inversion
// Output: none
void ILI9341_InvertDisplay(int i) ;

// graphics routines
// y coordinates 0 to 31 used for labels and messages
// y coordinates 32 to 159  128 pixels high
// x coordinates 0 to 127   128 pixels wide

// *************** ILI9341_PlotClear ********************
// Clear the graphics buffer, set X coordinate to 0
// This routine clears the display
// Inputs: ymin and ymax are range of the plot
// Outputs: none
void ILI9341_PlotClear(int32_t ymin, int32_t ymax);

// *************** ILI9341_PlotPoint ********************
// Used in the voltage versus time plot, plot one point at y
// It does output to display
// Inputs: y is the y coordinate of the point plotted
// Outputs: none
void ILI9341_PlotPoint(int32_t y);

// *************** ILI9341_PlotLine ********************
// Used in the voltage versus time plot, plot line to new point
// It does output to display
// Inputs: y is the y coordinate of the point plotted
// Outputs: none
void ILI9341_PlotLine(int32_t y);

// *************** ILI9341_PlotPoints ********************
// Used in the voltage versus time plot, plot two points at y1, y2
// It does output to display
// Inputs: y1 is the y coordinate of the first point plotted
//         y2 is the y coordinate of the second point plotted
// Outputs: none
void ILI9341_PlotPoints(int32_t y1,int32_t y2);

// *************** ILI9341_PlotBar ********************
// Used in the voltage versus time bar, plot one bar at y
// It does not output to display until RIT128x96x4ShowPlot called
// Inputs: y is the y coordinate of the bar plotted
// Outputs: none
void ILI9341_PlotBar(int32_t y);

// *************** ILI9341_PlotdBfs ********************
// Used in the amplitude versus frequency plot, plot bar point at y
// 0 to 0.625V scaled on a log plot from min to max
// It does output to display
// Inputs: y is the y ADC value of the bar plotted
// Outputs: none
void ILI9341_PlotdBfs(int32_t y);

// *************** ILI9341_PlotNext ********************
// Used in all the plots to step the X coordinate one pixel
// X steps from 0 to 127, then back to 0 again
// It does not output to display
// Inputs: none
// Outputs: none
void ILI9341_PlotNext(void);

void ILI9341_Line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);

// *************** ILI9341_PlotNextErase ********************
// Used in all the plots to step the X coordinate one pixel
// X steps from 0 to 127, then back to 0 again
// It clears the vertical space into which the next pixel will be drawn
// Inputs: none
// Outputs: none
void ILI9341_PlotNextErase(void);

// Used in all the plots to write buffer to LCD
// Example 1 Voltage versus time
//    ILI9341_PlotClear(0,4095);  // range from 0 to 4095
//    ILI9341_PlotPoint(data); ILI9341_PlotNext(); // called 128 times

// Example 2a Voltage versus time (N data points/pixel, time scale)
//    ILI9341_PlotClear(0,4095);  // range from 0 to 4095
//    {   for(j=0;j<N;j++){
//          ILI9341_PlotPoint(data[i++]); // called N times
//        }
//        ILI9341_PlotNext();
//    }   // called 128 times

// Example 2b Voltage versus time (N data points/pixel, time scale)
//    ILI9341_PlotClear(0,4095);  // range from 0 to 4095
//    {   for(j=0;j<N;j++){
//          ILI9341_PlotLine(data[i++]); // called N times
//        }
//        ILI9341_PlotNext();
//    }   // called 128 times

// Example 3 Voltage versus frequency (512 points)
//    perform FFT to get 512 magnitudes, mag[i] (0 to 4095)
//    ILI9341_PlotClear(0,1023);  // clip large magnitudes
//    {
//        ILI9341_PlotBar(mag[i++]); // called 4 times
//        ILI9341_PlotBar(mag[i++]);
//        ILI9341_PlotBar(mag[i++]);
//        ILI9341_PlotBar(mag[i++]);
//        ILI9341_PlotNext();
//    }   // called 128 times

// Example 4 Voltage versus frequency (512 points), dB scale
//    perform FFT to get 512 magnitudes, mag[i] (0 to 4095)
//    ILI9341_PlotClear(0,511);  // parameters ignored
//    {
//        ILI9341_PlotdBfs(mag[i++]); // called 4 times
//        ILI9341_PlotdBfs(mag[i++]);
//        ILI9341_PlotdBfs(mag[i++]);
//        ILI9341_PlotdBfs(mag[i++]);
//        ILI9341_PlotNext();
//    }   // called 128 times

// *************** ILI9341_OutChar ********************
// Output one character to the LCD
// Position determined by ILI9341_SetCursor command
// Color set by ILI9341_SetTextColor
// Inputs: 8-bit ASCII character
// Outputs: none
void ILI9341_OutChar(char ch);

//********ILI9341_OutString*****************
// Print a string of characters to the ILI9341 LCD.
// Position determined by ILI9341_SetCursor command
// Color set by ILI9341_SetTextColor
// The string will not automatically wrap.
// inputs: ptr  pointer to NULL-terminated ASCII string
// outputs: none
void ILI9341_OutString(char *ptr);

// ************** ILI9341_SetTextColor ************************
// Sets the color in which the characters will be printed
// Background color is fixed at black
// Input:  16-bit packed color
// Output: none
// ********************************************************
void ILI9341_SetTextColor(uint16_t color);

// *************** Output_Init ********************
// Standard device driver initialization function for printf
// Initialize ILI9341 LCD
// Inputs: none
// Outputs: none
void Output_Init(void);

// Clear display
void Output_Clear(void);

// Turn off display (low power)
void Output_Off(void);

// Turn on display
void Output_On(void);

// set the color for future output
// Background color is fixed at black
// Input:  16-bit packed color
// Output: none
void Output_Color(uint32_t newColor);

#endif
