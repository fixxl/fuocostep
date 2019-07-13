/*
 * adc.h
 * Einstellungen und Funktionen f�r den A/D-Umsetzer
 */

#ifndef ADC_H_
#define ADC_H_

void adc_init( void );
uint8_t adc_read( uint8_t channel );
void adc_cal( uint8_t channel );
#endif