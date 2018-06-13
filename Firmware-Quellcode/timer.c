/*
 * timer.c
 *
 *  Created on: 15.06.2013
 *      Author: Felix
 */

#include "global.h"

// Initialise timer 1
void timer1_init(void) {
	OCR1A   = 7499;
	TCCR1B |= (1 << WGM12); // CTC-Modus mit Prescaler 8 => f_C1 = 750 kHZ, T = 0,01 s = 10 ms
	TIMSK1 |= (1 << OCIE1A);
}

// Activate Timer 1 (Prescaler 8) => Overflow in CTC-mode after 1ms
void timer1_on(void) {
	TCCR1B |= (1 << CS11);
}

// Turn Timer 1 off
void timer1_off(void) {
	TCCR1B &= ~(1 << CS12 | 1 << CS11 | 1 << CS10);
}

// Reset Timer 1 (Counter to 0, clear overflow flag)
inline void timer1_reset(void) {
	TIFR1 = (1 << TOV1);
	TCNT1 = 0;
}
