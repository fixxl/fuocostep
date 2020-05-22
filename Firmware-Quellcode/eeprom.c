/*
 * eeprom.c
 *
 *  Created on: 15.06.2013
 *      Author: Felix
 */

#include "global.h"

// Read from EEPROM
uint8_t eeread( uint16_t address ) {
    uint8_t data;

    /* Wait for completion of previous write */
    while ( EECR & ( 1 << EEPE ) );

    /* Set up address register */
    EEAR = address;
    /* Start eeprom read by writing EERE */
    EECR |= ( 1 << EERE );
    /* Fetch data from Data Register */
    data = EEDR;
    EEAR = 0;
    return data;
}

// Write to EEPROM
void eewrite( uint8_t data, uint16_t address ) {
    /* Wait for completion of previous write */
    while ( EECR & ( 1 << EEPE ) );

    /* Set up address and Data Registers */
    EEAR = address;
    EEDR = data;
    /* Write logical one to EEMPE */
    EECR |= ( 1 << EEMPE );
    /* Start eeprom write by setting EEPE */
    EECR |= ( 1 << EEPE );
    EEAR  = 0;
}

uint16_t eeread16( uint16_t address ) {
    uint16_t data = 0;

    data |= ( eeread( address ) << 8 );
    data |= eeread( address + 1 );

    return data;
}

void eewrite16( uint16_t data, uint16_t address ) {
    eewrite( ( data >> 8 ) & 0xFF, address );
    eewrite( data & 0xFF, address + 1 );
}