/*
 * adc.c
 *
 *  Created on: 15.06.2013
 *      Author: Felix
 */

#include "global.h"

// De-Initialisation
void adc_deinit(void) {
	ADMUX  = 0;
	ADCSRA = 0;
}

// Initialisation
void adc_init(void) {
	// Comparator off
	ACSR |= 1 << ACD;

	// VCC as reference, select channel 4
	ADMUX |= ((1 << REFS0) | (1 << MUX2));

	// ADC-Prescaler = 32 (6MHz / 32 = 187kHz)
	ADCSRA |= ((1 << ADPS2) | (1 << ADPS0));

	// Turn on ADC and execute dummy-conversion
	ADCSRA |= 1 << ADEN;
	ADCSRA |= 1 << ADSC;

	// Wait for end of dummy conversion
	while (ADCSRA & (1 << ADSC)) ;

	(void) ADCW;
}

// Calculation of switch position
uint8_t adc_read(uint8_t channel) {
	uint16_t result = 0;
	uint16_t thresholds[12] = {47, 140, 233, 326, 419, 512, 605, 698, 791, 884, 977, 1024};

	ADMUX &= ~(0x07);
	ADMUX |= channel;

	for (uint8_t i = 8; i; i--) {
		ADCSRA |= 1 << ADSC;

		while (ADCSRA & (1 << ADSC)) ;

		result += ADCW;
	}
	result >>= 3;

	for(uint8_t i = 0; i < 12; i++) {
		if(result < thresholds[i]) return i;
	}

	return 0;
}
