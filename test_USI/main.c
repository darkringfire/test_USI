/*
 * test_USI.c
 *
 * Created: 01.06.2016 19:38:13
 * Author : dark
 */ 

#include <avr/io.h>
#include <util/delay.h>
#include "USI_I2C_Master.h"

#define ST_PORT PORTB
#define ST_DDR  DDRB
#define ST      4

#define MESSAGEBUF_SIZE       4

#define I2C_ADDR    0xAA

void init() {
    _delay_ms(100);
    ST_DDR = 1<<ST;
    ST_PORT = 1<<ST;
    _delay_ms(1);
}


int main(void) {
    uint8_t messageBuf[MESSAGEBUF_SIZE];
    
    init();
    USI_TWI_Master_Initialise();
    
    messageBuf[0] = I2C_ADDR | (0<<TWI_READ_BIT);
    USI_TWI_Start_Transceiver_With_Data(messageBuf, 1);
    while (1) {
    }
}

