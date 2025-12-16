#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <spiAVR.h>

#ifndef HELPER_H
#define HELPER_H

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

// SPI functions that simplifies the usage, automatically switching the ss line to low and up for me
void sendData(char data){
  PORTB = SetBit(PORTB, 1, 1); // turns A0 into 1

  PORTB = SetBit(PORTB, 2, 0); // selects the lcd screen
  SPI_SEND(data);
  PORTB = SetBit(PORTB, 2, 1); // deselects
}

// Similar SPI function as to above, just A0 is different
void sendCommand(char data){
  PORTB = SetBit(PORTB, 1, 0); // turns A0 into 0

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

#endif /* HEPLER_H */