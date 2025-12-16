#include "timerISR-Fixed.h"
//#include "usart_ATmega328p.h" // giving me huge errors that i dont want to deal with until display works
#include "spiAVR.h"
#include "acart040_helper_custom.h"


#define SWRESET 0x01 
#define SLPOUT  0x11
#define COLMOD  0x3A
#define DISPON  0x29
#define NORON   0x13
#define MADCTL  0x36
#define CASET   0x2A // for setting the x axis
#define RASET   0x2B // for setting the y axis
#define RAMWR   0x2C // for actually drawing

/*
Colors used: (B G R)
Background Gray: 19 18 18
Square Gray: 60 58 58
Square Yellow: 59 159 181
Square Green: 78 141 83
Text White: 255 255 255
*/

void TimerISR(){
  // bleehhhhhh
  // will be implemented later just here for now for no errors
}

// reminder, the display is 128 * 128
void drawRectangle(char xStart, char xEnd, char yStart, char yEnd){ // test funtion to write a gray box, going to test for the gray backgrounds later

  sendCommand(CASET); // x boundary sets
  sendData(0);
  sendData(xStart);
  sendData(0);
  sendData(xEnd);
 
  sendCommand(RASET); // y boundary sets
  sendData(0);
  sendData(yStart);
  sendData(0);
  sendData(yEnd);

  // this creates a frame we can draw pixels in :P

  int area = (xEnd - xStart + 1) * (yEnd - yStart + 1); // change of distance to determine area, will find algorithm to fill in rectangle tonight :P

  sendCommand(RAMWR); // start drawing in new frame
  for (int i = 0; i < area; i++){
    sendData(59); // b
    sendData(159); // g 
    sendData(181); // r
  }
}

void HardwareReset(){
  //setResetPinToLow; // setbit(reset pin to 0)
  PORTB = SetBit(PORTB, 0, 0);
  _delay_ms(200);
  //setResetPinToHigh; // setbit(reset pin to 1)
  PORTB = SetBit(PORTB, 0, 1);
  _delay_ms(200);
}

void ST7735_init(){
  HardwareReset();

  sendCommand(SWRESET);
  _delay_ms(150);

  sendCommand(SLPOUT);
  _delay_ms(200);

  sendCommand(COLMOD);
  sendData(0x06); //for 18 bit color mode. You can pick any color mode you want
  _delay_ms(10);

  sendCommand(DISPON);
  _delay_ms(200);
}

// RST is PORTB0
// A0 is PORTB1
// SS is PORTB2

int main(void){
  DDRB = 0x2F;
  PORTB = 0x10; 

  SPI_INIT();
  ST7735_init();

  drawRectangle(0, 128, 0, 128);
  while(1){}
}