
// Permission to copy is granted provided that this header remains intact. 
// This software is provided with no warranties.

////////////////////////////////////////////////////////////////////////////////

#ifndef USART_328p
#define USART_328p

#include <avr/io.h>
// USART Setup Values
#define F_CPU 16000000UL // Assume uC operates at 16MHz
#define BAUD_RATE 9600//38400
#define BAUD_PRESCALE (((F_CPU / (BAUD_RATE * 16UL))) - 1)

////////////////////////////////////////////////////////////////////////////////
//Functionality - Initializes TX and RX on PORT D
//Returns: None
void initUSART()
{
		// Turn on the reception circuitry of USART0
		// Turn on receiver and transmitter
		// Use 8-bit character sizes 
		UCSR0B |= (1 << RXEN0)  | (1 << TXEN0);
		UCSR0C |= (1 << UCSZ00) | (1 << UCSZ01);
		// Load lower 8-bits of the baud rate value into the low byte of the UBRR0 register
		UBRR0L = BAUD_PRESCALE;
		// Load upper 8-bits of the baud rate value into the high byte of the UBRR0 register
		UBRR0H = (BAUD_PRESCALE >> 8);
}
////////////////////////////////////////////////////////////////////////////////
//Functionality - checks if USART is ready to send
//Returns: 1 if true else 0
unsigned char USART_IsSendReady()
{
	return UCSR0A & (1 << UDRE0);
}
////////////////////////////////////////////////////////////////////////////////
//Functionality - checks if USART has successfully transmitted data
//Returns: 1 if true else 0
unsigned char USART_HasTransmitted()
{
	return UCSR0A & (1 << TXC0);
}
////////////////////////////////////////////////////////////////////////////////
// **** WARNING: THIS FUNCTION BLOCKS MULTI-TASKING; USE WITH CAUTION!!! ****
//Functionality - checks if USART has recieved data
//Returns: 1 if true else 0
unsigned char USART_HasReceived()
{
	return UCSR0A & (1 << RXC0);
}
////////////////////////////////////////////////////////////////////////////////
//Functionality - Flushes the data register
//Returns: None
void USART_Flush()
{
	static unsigned char dummy;
	
	while ( UCSR0A & (1 << RXC0) ) { dummy = UDR0; }
	
}
////////////////////////////////////////////////////////////////////////////////
// **** WARNING: THIS FUNCTION BLOCKS MULTI-TASKING; USE WITH CAUTION!!! ****
//Functionality - Sends an 8-bit char value
//Parameter: Takes a single unsigned char value
//Returns: None
void USART_Send(unsigned char sendMe)
{

	while( !(UCSR0A & (1 << UDRE0)) );
	UDR0 = sendMe;
	
}
////////////////////////////////////////////////////////////////////////////////
// **** WARNING: THIS FUNCTION BLOCKS MULTI-TASKING; USE WITH CAUTION!!! ****
//Functionality - receives an 8-bit char value
//Returns: Unsigned char data from the receive buffer
unsigned char USART_Receive()
{

	while ( !(UCSR0A & (1 << RXC0)) ); // Wait for data to be received
	return UDR0; // Get and return received data from buffer
	
}

#endif USART_328p