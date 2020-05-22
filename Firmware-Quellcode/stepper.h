/*
 * pyro.h
 *
 *  Created on: 10.07.2013
 *      Author: Felix
 */

#ifndef STEPPER_H_
#define STEPPER_H_

#define DEBUGMODE          0

#define MAX_COM_ARRAYSIZE      30

// Key switch location
#define KEYPORT            D
#define KEYNUM             3

// Optocoupler trigger input
#define OPTOPORT           D
#define OPTONUM            2

// P-Channel-Switch
#define MOSSWITCHPORT      C
#define MOSSWITCHNUM       5

// Igniter time durations
#define EMATCH_TIME        2
#define TALON_TIME         200
#define IGNITER_TIME       EMATCH_TIME

#define TRIGGERVAL         60000

// Ceiled duration of byte transmission in microseconds
#define   BYTE_DURATION_US ( 8 * ( 1000000UL + BITRATE ) / BITRATE )

// Bitflags
typedef union {
    struct {
        unsigned uart_active    : 1;
        unsigned uart_config    : 1;
        unsigned fire           : 1;
        unsigned finish_firing  : 1;
        unsigned is_fire_active : 1;
        unsigned step           : 1;
        unsigned list           : 1;
        unsigned setup          : 1;
        unsigned adc            : 1;
        unsigned edit           : 1;
        unsigned quickread      : 1;
        unsigned reset_device   : 1;
        unsigned clear_list     : 1;
        unsigned quickwrite     : 1;
        unsigned remote         : 1;
    }        b;
    uint16_t complete;
} bitfeld_t;

#define KEY_DDR           DDR( KEYPORT )
#define KEY_PIN           PIN( KEYPORT )
#define KEY_PORT          PORT( KEYPORT )
#define KEY_NUMERIC       NUMPORT( KEYPORT )
#define KEY               KEYNUM

#if KEY_NUMERIC == DPORT
    #define KEYINT        PCINT2_vect
    #define KEYMSK        PCMSK2
    #define KEYPCIE       PCIE2
#elif KEY_NUMERIC == BPORT
    #define KEYINT        PCINT0_vect
    #define KEYMSK        PCMSK0
    #define KEYPCIE       PCIE0
#else
    #define KEYINT        PCINT1_vect
    #define KEYMSK        PCMSK1
    #define KEYPCIE       PCIE1
#endif

#define OPTO_DDR          DDR( OPTOPORT )
#define OPTO_PIN          PIN( OPTOPORT )
#define OPTO_PORT         PORT( OPTOPORT )
#define OPTO_NUMERIC      NUMPORT( OPTOPORT )
#define OPTO              OPTONUM
#if OPTO_NUMERIC == DPORT
    #define OPTOINT       PCINT2_vect
    #define OPTOMSK       PCMSK2
    #define OPTOPCIE      PCIE2
#elif OPTO_NUMERIC == BPORT
    #define OPTOINT       PCINT0_vect
    #define OPTOMSK       PCMSK0
    #define OPTOPCIE      PCIE0
#else
    #define OPTOINT       PCINT1_vect
    #define OPTOMSK       PCMSK1
    #define OPTOPCIE      PCIE1
#endif

#if ( KEY_NUMERIC == OPTO_NUMERIC )
    #define COMMONINT     1
    #define INTERRUPT_PIN OPTO_PIN
#else
    #define COMMONINT     0
#endif

#define MOSSWITCH_DDR     DDR( MOSSWITCHPORT )
#define MOSSWITCH_PIN     PIN( MOSSWITCHPORT )
#define MOSSWITCH_PORT    PORT( MOSSWITCHPORT )
#define MOSSWITCH         MOSSWITCHNUM


// Funktionsprototypen
void    wdt_init( void ) __attribute__( ( naked ) ) __attribute__( ( section( ".init1" ) ) );
void    create_symbols( void );
uint8_t asciihex( char inp );
void    key_init( void );
uint8_t debounce( volatile uint8_t *port, uint8_t pin );
#endif /* STEPPER_H_ */
