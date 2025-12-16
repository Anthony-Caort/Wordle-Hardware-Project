#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <spiAVR.h>
#include <string.h>
#include <acart040_character_sheet.h>
#include <serialATmega-4.h>

#ifndef HELPER_H
#define HELPER_H

#define CASET   0x2A // for setting the x axis
#define RASET   0x2B // for setting the y axis
#define RAMWR   0x2C // for actually drawing

// for initialization
#define SWRESET 0x01 
#define SLPOUT  0x11
#define COLMOD  0x3A
#define DISPON  0x29
#define NORON   0x13
#define MADCTL  0x36

// offset for background
#define X_OFFSET 2
#define Y_OFFSET 4

enum colors{gray, yellow, green, darkGray, white, red} currColor;

typedef struct _letter{ // made to keep track of what's right and what's not, used in a 5 element array
  colors currentColor = gray; // current Color the letter is
} letterC;

//Functionality - finds the greatest common divisor of two values
//Parameter: Two long int's to find their GCD
//Returns: GCD else 0
unsigned long int findGCD(unsigned long int a, unsigned long int b)
{
	unsigned long int c;
	while(1){
		c = a % b;
		if( c == 0 ) { return b; }
		a = b;
		b = c;
	}
	return 0;
}

unsigned char SetBit(unsigned char x, unsigned char k, unsigned char b) {
   return (b ?  (x | (0x01 << k))  :  (x & ~(0x01 << k)) );
              //   Set bit to 1           Set bit to 0
}

unsigned char GetBit(unsigned char x, unsigned char k) {
   return ((x & (0x01 << k)) != 0);
}

// ADC related functions, mainly for the buzzer (i think)
void ADC_init() {
  ADMUX = (1<<REFS0);
	ADCSRA|= (1<<ADEN) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);
	// ADEN: setting this bit enables analog-to-digital conversion.
	// ADSC: setting this bit starts the first conversion.
	// ADATE: setting this bit enables auto-triggering. Since we are
	//        in Free Running Mode, a new conversion will trigger whenever
	//        the previous conversion completes.
}

unsigned int ADC_read(unsigned char chnl){
  	uint8_t low, high;

  	ADMUX  = (ADMUX & 0xF8) | (chnl & 7);
  	ADCSRA |= 1 << ADSC ;
  	while((ADCSRA >> ADSC)&0x01){}
  
  	low  = ADCL;
	high = ADCH;

	return ((high << 8) | low) ;
}

// SPI functions that simplifies the usage, automatically switching the ss line to low and up for me
void sendData(char data){
  PORTD = SetBit(PORTD, 7, 1); // turns A0 into 1

  PORTB = SetBit(PORTB, 2, 0); // selects the lcd screen
  SPI_SEND(data);
  PORTB = SetBit(PORTB, 2, 1); // deselects
}

// Similar SPI function as to above, just A0 is different
void sendCommand(char data){
  PORTD = SetBit(PORTD, 7, 0); // turns A0 into 0

  PORTB = SetBit(PORTB, 2, 0); // selects the lcd screen
  SPI_SEND(data);
  PORTB = SetBit(PORTB, 2, 1); // deselects
}

//aFirst/Second: First range of values
//bFirst/Second: Range of values to map to
//inVal: value being mapped
unsigned int map_value(unsigned int aFirst, unsigned int aSecond, unsigned int bFirst, unsigned int bSecond, unsigned int inVal)
{
	return bFirst + (long((inVal - aFirst))*long((bSecond-bFirst)))/(aSecond - aFirst);
}

/*
  Start of drawing related functions :P, will add more after testing is done
*/

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

void drawGray(){
  sendData(60); // b
  sendData(58); // g
  sendData(58); // r
}

void drawDarkGray(){
  sendData(19); // b
  sendData(18); // g
  sendData(18); // r
}

void drawYellow(){
  sendData(59); // b
  sendData(159); // g
  sendData(181); // r
}

void drawGreen(){
  sendData(78); // b
  sendData(141); // g
  sendData(83); // r
}

void drawWhite(){
  sendData(255); // b
  sendData(255); // g
  sendData(255); // r
}

void drawRed(){
  sendData(0); // b
  sendData(0); // g
  sendData(255); // r
}

// reminder, the display is 128 * 128
void drawBackground(char xStart, char xEnd, char yStart, char yEnd){ // test funtion to write a gray box, going to test for the gray backgrounds later

  sendCommand(CASET); // x boundary sets
  sendData(0);
  sendData(xStart);
  sendData(0);
  sendData(xEnd + X_OFFSET);
 
  sendCommand(RASET); // y boundary sets
  sendData(0);
  sendData(yStart);
  sendData(0);
  sendData(yEnd + Y_OFFSET);

  // this creates a frame we can draw pixels in :P

  int area = (xEnd - (xStart + X_OFFSET) + 1) * (yEnd - (yStart + Y_OFFSET) + 1); // change of distance to determine area, will find algorithm to fill in rectangle tonight :P

  sendCommand(RAMWR); // start drawing in new frame
  for (int i = 0; i < area; i++){
    drawDarkGray();
  }
}

void drawFrame(char xStart, char xEnd, char yStart, char yEnd){
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

  sendCommand(RAMWR); // start drawing in new frame
  for (int i = 0; i < 17; i++){
    for (int j = 0; j < 17; j++){
      if (i == 0 || i == 16 || j == 0 || j == 16){
        drawGray();
      }
      else{
        drawDarkGray();
      }
    }
  }
}

//first 4 inputs are self-explanatory but letter[] is a pointer to the character arrays, and currLetter is the current letter index we want to print
void drawLetter(char xStart, char xEnd, char yStart, char yEnd, unsigned short* letter[], int currLetter, colors currColor){
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

  sendCommand(RAMWR);

  for (int i = 0; i < 15; i++){ // i think its only O(n)
    for (int j = 15; j != 0; j--){
      if ((letter[currLetter][i] >> j) & 0x0001){
        drawWhite();
      }
      else{
        if (currColor == yellow) {drawYellow();}
        else if (currColor == gray) {drawGray();}
        else if (currColor == green) {drawGreen();}
        else if (currColor == darkGray) {drawDarkGray();}
        else if (currColor == white) {drawWhite();}
        else if (currColor == red) {drawRed();}
      }
    }
  }
}

void drawStart(char xStart, char xEnd, char yStart, char yEnd){
  char startX = xStart;
  char endX = xEnd;
  char startY = yStart;
  char endY = yEnd;
  
  // drawing the frames
  for (int i = 0; i < 31; i++){
    drawFrame(startX, endX, startY, endY);
    startX = endX + 5;
    endX = startX + 16;
    if ((i % 5) == 0 && i != 0){
      startX = xStart;
      endX = xEnd;
      startY = endY + 5;
      endY = startY + 16;
    }
  }

  char xStartL = xStart - 20;
  char yStartL = yStart + 21;
  char xEndL = xStartL + 14;
  char yEndL = yStartL + 14;

  // for drawing the starting user letters
  // all letters will start as A
  for (int i = 0; i < 5; ++i){
    drawLetter(xStartL, xEndL, yStartL, yEndL, letters, 0, darkGray);
    yStartL = yEndL + 5;
    yEndL = yStartL + 14;
  }
  // we are free balling
}

void drawGuessColor(char xStart, char xEnd, char yStart, char yEnd, colors fillIn){
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

  int area = (xEnd - xStart + 1) * (yEnd - yStart + 1);

  sendCommand(RAMWR); // start drawing in new frame
  for (int i = 0; i < area; i++){
    if (fillIn == yellow) {drawYellow();}
    else if (fillIn == gray) {drawGray();}
    else if (fillIn == green) {drawGreen();}
    else if (fillIn == darkGray) {drawDarkGray();}
  }
}

void drawIntroText(int xPos[]){ // im tired
  drawLetter(xPos[0], xPos[0] + 14, 110, 124, letters, 22, green); // w
  drawLetter(xPos[1], xPos[1] + 14, 110, 124, letters, 14, green); // o
  drawLetter(xPos[2], xPos[2] + 14, 110, 124, letters, 17, green); // r
  drawLetter(xPos[3], xPos[3] + 14, 110, 124, letters, 3, green); // d
  drawLetter(xPos[4], xPos[4] + 14, 110, 124, letters, 11, green); // l
  drawLetter(xPos[4] - 21, xPos[4] - 7, 110, 124, letters, 4, green); // e :P
}

/*
  Wordle.cpp related functions
*/

void resetColors(letterC currLetters[]) {
    for (int i = 0; i < 5; i++) {
        currLetters[i].currentColor = gray;
    }
}

bool validationProcess(char check[], const char* wordList[]){ // basically just binary search lol
  int left = 0;
  int right = 99; // very last index of 
  int mid;
  int found;

  while (left <= right){
    mid = left + (right - left) / 2;
    found = memcmp(check, wordList[mid], 5);

    if (found == 0){
      return true;
    }
    else if (found > 0){ // if the current word is above the mid
      left = mid + 1; 
    }
    else {right = mid - 1;} // if the current word is below the mid
  }

  return false;
}

void correctionChecks(char inputWord[], char correctWord[], letterC currCorrection[]){
  // first check, check if the current word has any of the correct letters
  // I think its O(n^2) but we're only doing 25 runs and never goes above that so I think its fine

  int used[] = {0, 0, 0, 0, 0}; // used for yellow checking, 1 if its yellow or green and we skip if that index is 1

  for (int i = 0; i < 5; ++i){
    if (inputWord[i] == correctWord[i]){
      currCorrection[i].currentColor = green;
      used[i] = 2;
    }
  }

  for (int i = 0; i < 5; ++i){
    if (used[i] != 2){
      for (int j = 0; j < 5; ++j){
        if (inputWord[i] == correctWord[j] && used[j] == 0){
          currCorrection[i].currentColor = yellow;
          used[j] = 1;
        }
      }
    }
  }
  // any letter not changed from both checks will remain gray
}

#endif /* HEPLER_H */