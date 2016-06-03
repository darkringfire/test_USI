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

#define MESSAGEBUF_SIZE       10

#define I2C_ADDR    0xBE

void init() {
	DDRB = 1<<4;
	PORTB = 1<<4;
	
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

	/* Wait for empty transmit buffer */
	while ( !( UCSRA & (1<<UDRE)) ); UDR = 0x00;
}


int main(void) {
	
	
	
		
	
	
	
	
	
    uint8_t messageBuf[MESSAGEBUF_SIZE];
	uint8_t memAddr;
	//uint8_t chipAddr;
	//uint8_t addrOK;
	#define VAL 0xAA
    
    init();
	
	
    USI_TWI_Master_Initialise();

	if (0) {
		messageBuf[0] = I2C_ADDR;
		messageBuf[1] = 0x28;
		messageBuf[2] = VAL;
		messageBuf[3] = VAL;
		messageBuf[4] = VAL;
		messageBuf[5] = VAL;
		messageBuf[6] = VAL;
		messageBuf[7] = VAL;
		messageBuf[8] = VAL;
		messageBuf[9] = VAL;
		USI_TWI_Start_Transceiver_With_Data(messageBuf, 10, 0);
		USI_TWI_Master_Stop();
		while ( !USI_TWI_Start_Transceiver_With_Data(messageBuf, 1, 0) );
		USI_TWI_Master_Stop();
	}
    
	if (0) {
		messageBuf[0] = I2C_ADDR;
		messageBuf[2] = VAL;
		messageBuf[3] = VAL;
		messageBuf[4] = VAL;
		messageBuf[5] = VAL;
		messageBuf[6] = VAL;
		messageBuf[7] = VAL;
		messageBuf[8] = VAL;
		messageBuf[9] = VAL;
		memAddr = 0;
		for (memAddr = (0x28>>3); memAddr < (0x48>>3); memAddr++) {
			messageBuf[1] = (memAddr << 3);
			
			if ( USI_TWI_Start_Transceiver_With_Data(messageBuf, 10, 0) ) {
				USI_TWI_Master_Stop();
				_delay_ms(12);
			} else {
				while ( !( UCSRA & (1<<UDRE)) ); UDR = memAddr;
				USI_TWI_Master_Stop();
			}
		}
	}
	
	if (1) {
		messageBuf[0] = I2C_ADDR | (1<<TWI_READ_BIT);
		for (memAddr = (0x28>>3); memAddr < (0x48>>3); memAddr++) {
			messageBuf[1] = (memAddr << 3);
			USI_TWI_Start_Transceiver_With_Data(messageBuf, 2, 8);
			USI_TWI_Master_Stop();
		}
	}
	
    while (1) {
    }
}

