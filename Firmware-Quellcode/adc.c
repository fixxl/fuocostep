/*
 * adc.c
 *
 *  Created on: 15.06.2013
 *      Author: Felix
 */

#include "global.h"

// Initialisation
void adc_init( void ) {
    // Comparator off
    ACSR |= 1 << ACD;

    // VCC as reference, select channel 4
    ADMUX = ( ( 1 << REFS0 ) | ( 1 << MUX2 ) );

    // ADC-Prescaler = 32 (6MHz / 32 = 187kHz)
    ADCSRA |= ( ( 1 << ADPS2 ) | ( 1 << ADPS0 ) );

    // Turn on ADC and execute dummy-conversion
    ADCSRA |= 1 << ADEN;
    ADCSRA |= 1 << ADSC;

    // Wait for end of dummy conversion
    while ( ADCSRA & ( 1 << ADSC ) );

    volatile uint16_t dummyval = ADCW;
    (void) dummyval;
}

// Calculation of switch position
uint8_t adc_read( uint8_t channel ) {
    uint16_t       result         = 0;
    const uint16_t thresholds[12] = { 47, 140, 233, 326, 419, 512, 605, 698, 791, 884, 977, 1024 };

    ADMUX &= 0xF8;
    ADMUX |= channel;

    for ( uint8_t i = 32; i; i-- ) {
        ADCSRA |= 1 << ADSC;

        while ( ADCSRA & ( 1 << ADSC ) );

        result += ADCW;
    }
    result  += 16;
    result >>= 5;

    for ( uint8_t i = 0; i < 12; i++ ) {
        if ( result < thresholds[i] ) {
            return i;
        }
    }

    return 0;
}

// Calculation of switch position
void adc_cal( uint8_t channel ) {
    uint16_t result = 0, singleresult;
    // uint16_t thresholds[12] = {47, 140, 233, 326, 419, 512, 605, 698, 791, 884, 977, 1024};

    ADMUX &= 0xF8;
    ADMUX |= channel;

    for ( uint8_t i = 32; i; i-- ) {
        ADCSRA |= 1 << ADSC;

        while ( ADCSRA & ( 1 << ADSC ) );

        singleresult = ADCW;
        result      += singleresult;
        uart_shownum( singleresult, 'd' );
        uart_puts_P( PSTR( "\r\n" ) );
    }
    result  += 16;
    result >>= 5;
    uart_puts_P( PSTR( "\r\nDurchschnitt: " ) );
    uart_shownum( result, 'd' );
    uart_puts_P( PSTR( "\r\nADMUX: " ) );
    uart_shownum( ADMUX, 'h' );
}