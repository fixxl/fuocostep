/*
 * global.h
 *
 *  Created on: 15.06.2013
 *      Author: Felix
 */

#ifndef GLOBAL_H_
#define GLOBAL_H_

#ifndef MCU
    #define MCU "atmega328p"
#endif


#define STRINGIZE( x )          # x
#define STRINGIZE_VALUE_OF( x ) STRINGIZE( x )

// Includes und Defines
#include <avr/io.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <string.h>
#include "portmakros.h"
#include "timer.h"
#include "eeprom.h"
#include "stepper.h"
#include "leds.h"
#include "timeparser.h"
#include "uart.h"
#include "terminal.h"
#include "adc.h"
#include "shiftregister.h"

// Global Variables
// extern volatile uint8_t transmit_flag, key_flag;
// extern volatile uint16_t clear_lcd_tx_flag, clear_lcd_rx_flag, hist_del_flag;
#endif /* GLOBAL_H_ */