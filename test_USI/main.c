/*
 * test_USI.c
 *
 * Created: 01.06.2016 19:38:13
 * Author : dark
 */ 

#include <avr/io.h>
#include <avr/cpufunc.h>
#include <util/delay.h>
#include "USI_I2C_Master.h"

#define baud 0

#define ST_PORT PORTB
#define ST_DDR  DDRB
#define ST      4

#define MESSAGEBUF_SIZE       4

#define I2C_ADDR    0b11011000

void init() {
    _delay_ms(10);
    ST_DDR = 1<<ST;
    ST_PORT = 1<<ST;
    //_delay_ms(1);

	
	/* Set baud rate */
	UBRRH = (unsigned char)(baud>>8);
	UBRRL = (unsigned char)baud;
	UCSRA = (1<<U2X);
	/* Enable receiver and transmitter */
	UCSRB = (1<<RXEN)|(1<<TXEN);
	/* Set frame format: 8data, 2stop bit */
	UCSRC = (1<<USBS)|(3<<UCSZ0);

	DDRB = 1<<4;
	//PORTB = 1<<4;
	
	/* Wait for empty transmit buffer */
	while ( !( UCSRA & (1<<UDRE)) ); UDR = 0x00;
}


int main(void) {
	
	
	
		
	
	
	
	
	
    uint8_t messageBuf[MESSAGEBUF_SIZE];
    
    init();
	
	
    USI_TWI_Master_Initialise();
    
    messageBuf[0] = I2C_ADDR;
    messageBuf[1] = 0xAA;
    USI_TWI_Start_Transceiver_With_Data(messageBuf, 2);
    messageBuf[0] = I2C_ADDR | (1<<TWI_READ_BIT);
    USI_TWI_Start_Transceiver_With_Data(messageBuf, 2);
    USI_TWI_Master_Stop();
    while (1) {
    }
}

