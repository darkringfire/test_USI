/*
 * USI_I2C_Master.c
 *
 * Created: 01.06.2016 21:08:40
 *  Author: dark
 */ 

#include "USI_I2C_Master.h"
#include <avr/io.h>
#include <avr/cpufunc.h>
#include <util/delay.h>

uint8_t USI_TWI_Master_Transfer( uint8_t temp );

union  USI_TWI_state
{
    uint8_t errorState;         // Can reuse the TWI_state for error states due to that it will not be need if there exists an error.
    struct
    {
        uint8_t addressMode         : 1;
        uint8_t masterWriteDataMode : 1;
        uint8_t unused              : 6;
    };
}   USI_TWI_state;

/*---------------------------------------------------------------
 USI TWI single master initialization function
---------------------------------------------------------------*/
void USI_TWI_Master_Initialise( void )
{
    PORT_USI |= (1<<PIN_USI_SDA);           // Enable pullup on SDA, to set high as released state.
    PORT_USI |= (1<<PIN_USI_SCL);           // Enable pullup on SCL, to set high as released state.
    
    USIDR    =  0xFF;                       // Preload dataregister with "released level" data.
    USICR    =  (0<<USISIE)|(0<<USIOIE)|                            // Disable Interrupts.
                (1<<USIWM1)|(0<<USIWM0)|                            // Set USI in Two-wire mode.
                (1<<USICS1)|(0<<USICS0)|(1<<USICLK)|                // Software stobe as counter clock source
                (0<<USITC);
    USISR   =   (1<<USISIF)|(1<<USIOIF)|(1<<USIPF)|(1<<USIDC)|      // Clear flags,
                (0x0<<USICNT0);                                     // and reset counter.

    DDR_USI  |= (1<<PIN_USI_SCL);           // Enable SCL as output.
    DDR_USI  |= (1<<PIN_USI_SDA);           // Enable SDA as output.
    
}

/*---------------------------------------------------------------
 USI Transmit and receive function. LSB of first byte in data 
 indicates if a read or write cycles is performed. If set a read
 operation is performed.

 Function generates (Repeated) Start Condition, sends address and
 R/W, Reads/Writes Data, and verifies/sends ACK.
 
 Success or error code is returned. Error codes are defined in 
 USI_TWI_Master.h
---------------------------------------------------------------*/
uint8_t USI_TWI_Start_Transceiver_With_Data( uint8_t *msg, uint8_t msgSize)
{
    uint8_t tempUSISR_8bit = (1<<USISIF)|(1<<USIOIF)|(1<<USIPF)|(1<<USIDC)|      // Prepare register value to: Clear flags, and
                             (0x0<<USICNT0);                                     // set USI to shift 8 bits i.e. count 16 clock edges.
    uint8_t tempUSISR_1bit = (1<<USISIF)|(1<<USIOIF)|(1<<USIPF)|(1<<USIDC)|      // Prepare register value to: Clear flags, and
                             (0xE<<USICNT0);                                     // set USI to shift 1 bit i.e. count 2 clock edges.

    USI_TWI_state.errorState = 0;
    USI_TWI_state.addressMode = TRUE;

    #ifdef PARAM_VERIFICATION
    if(msg > (uint8_t*)RAMEND)                 // Test if address is outside SRAM space
    {
        USI_TWI_state.errorState = USI_TWI_DATA_OUT_OF_BOUND;
        return (FALSE);
    }
    if(msgSize <= 1)                                 // Test if the transmission buffer is empty
    {
        USI_TWI_state.errorState = USI_TWI_NO_DATA;
        return (FALSE);
    }
    #endif

    #ifdef NOISE_TESTING
    if( USISR & (1<<USISIF) )                       // Test if any unexpected conditions have arrived prior to this execution.
    {
        USI_TWI_state.errorState = USI_TWI_UE_START_CON;
        return (FALSE);
    }
    if( USISR & (1<<USIPF) )
    {
        USI_TWI_state.errorState = USI_TWI_UE_STOP_CON;
        return (FALSE);
    }
    if( USISR & (1<<USIDC) )
    {
        USI_TWI_state.errorState = USI_TWI_UE_DATA_COL;
        return (FALSE);
    }
    #endif

    if ( !(*msg & (1<<TWI_READ_BIT)) )                // The LSB in the address byte determines if is a masterRead or masterWrite operation.
    {
        USI_TWI_state.masterWriteDataMode = TRUE;
    }

    /* Release SCL to ensure that (repeated) Start can be performed */
    PORT_USI |= (1<<PIN_USI_SCL);                     // Release SCL.
    while( !(PIN_USI & (1<<PIN_USI_SCL)) );          // Verify that SCL becomes high.
    _delay_us( T_TWI/2 );

    /* Generate Start Condition */
    PORT_USI &= ~(1<<PIN_USI_SDA);                    // Force SDA LOW.
    _delay_us( T_TWI/2 );
    PORT_USI &= ~(1<<PIN_USI_SCL);                    // Pull SCL LOW.
    PORT_USI |= (1<<PIN_USI_SDA);                     // Release SDA.

    #ifdef SIGNAL_VERIFY
    if( !(USISR & (1<<USISIF)) )
    {
        USI_TWI_state.errorState = USI_TWI_MISSING_START_CON;
        return (FALSE);
    }
    #endif

    /*Write address and Read/Write data */
    do 
    {
        /* If masterWrite cycle (or initial address transmission)*/
        if (USI_TWI_state.addressMode || USI_TWI_state.masterWriteDataMode)
        {
            /* Write a byte */
            PORT_USI &= ~(1<<PIN_USI_SCL);                // Pull SCL LOW.
            USIDR     = *(msg++);                        // Setup data.
            USI_TWI_Master_Transfer( tempUSISR_8bit );    // Send 8 bits on bus.
            
            /* Clock and verify (N)ACK from slave */
            DDR_USI  &= ~(1<<PIN_USI_SDA);                // Enable SDA as input.
            DDRA |= (1<<1);
            if( USI_TWI_Master_Transfer( tempUSISR_1bit ) & (1<<TWI_NACK_BIT) )
            {
                if ( USI_TWI_state.addressMode ) {
                    USI_TWI_state.errorState = USI_TWI_NO_ACK_ON_ADDRESS;
                } else {
                    USI_TWI_state.errorState = USI_TWI_NO_ACK_ON_DATA;
                }          
                DDRA &= ~(1<<1);
                //USI_TWI_Master_Stop();
                return (FALSE);
            }
            DDRA &= ~(1<<1);
            USI_TWI_state.addressMode = FALSE;            // Only perform address transmission once.
        }
        /* Else masterRead cycle*/
        else
        {
            /* Read a data byte */
            DDR_USI   &= ~(1<<PIN_USI_SDA);               // Enable SDA as input.
            *(msg++)  = USI_TWI_Master_Transfer( tempUSISR_8bit );

            /* Prepare to generate ACK (or NACK in case of End Of Transmission) */
            if( msgSize == 1)                            // If transmission of last byte was performed.
            {
                USIDR = 0xFF;                              // Load NACK to confirm End Of Transmission.
            }
            else
            {
                USIDR = 0x00;                              // Load ACK. Set data register bit 7 (output for SDA) low.
            }
            USI_TWI_Master_Transfer( tempUSISR_1bit );   // Generate ACK/NACK.
        }
    } while ( --msgSize) ;                             // Until all data sent/received.
    
    //USI_TWI_Master_Stop();                           // Send a STOP condition on the TWI bus.

    /* Transmission successfully completed*/
    return (TRUE);
}

/*---------------------------------------------------------------
 Core function for shifting data in and out from the USI.
 Data to be sent has to be placed into the USIDR prior to calling
 this function. Data read, will be returned from the function.
---------------------------------------------------------------*/
uint8_t USI_TWI_Master_Transfer( uint8_t temp )
{
  USISR = temp;                                     // Set USISR according to temp.
                                                    // Prepare clocking.
  temp  =  (0<<USISIE)|(0<<USIOIE)|                 // Interrupts disabled
           (1<<USIWM1)|(0<<USIWM0)|                 // Set USI in Two-wire mode.
           (1<<USICS1)|(0<<USICS0)|(1<<USICLK)|     // Software clock strobe as source.
           (1<<USITC);                              // Toggle Clock Port.
  do
  {
    //_delay_us( T2_TWI/4 );              
    _delay_us( T_TWI/2 );
    USICR = temp;                          // Generate positive SCL edge.
    while( ~PIN_USI & (1<<PIN_USI_SCL) ); // Wait for SCL to go high.
    //_delay_us( T4_TWI/4 );              
    _delay_us( T_TWI/2 );
    USICR = temp;                          // Generate negative SCL edge.
  } while( ~USISR & (1<<USIOIF) );        // Check for transfer complete.
  
  //_delay_us( T2_TWI/4 );                
  _delay_us( T_TWI/2 );
  temp  = USIDR;                           // Read out data.
  USIDR = 0xFF;                            // Release SDA.
  DDR_USI |= (1<<PIN_USI_SDA);             // Enable SDA as output.

  return temp;                             // Return the data from the USIDR
}

/*---------------------------------------------------------------
 Function for generating a TWI Stop Condition. Used to release 
 the TWI bus.
---------------------------------------------------------------*/
uint8_t USI_TWI_Master_Stop( void )
{
  PORT_USI &= ~(1<<PIN_USI_SDA);           // Pull SDA low.
  PORT_USI |= (1<<PIN_USI_SCL);            // Release SCL.
  while( !(PIN_USI & (1<<PIN_USI_SCL)) );  // Wait for SCL to go high.
  //_delay_us( T4_TWI/4 );               
  _delay_us( T_TWI/2 );
  PORT_USI |= (1<<PIN_USI_SDA);            // Release SDA.
  //_delay_us( T2_TWI/4 );                
  _delay_us( T_TWI/2 );
  
#ifdef SIGNAL_VERIFY
  if( !(USISR & (1<<USIPF)) )
  {
    USI_TWI_state.errorState = USI_TWI_MISSING_STOP_CON;    
    return (FALSE);
  }
#endif

  return (TRUE);
}
