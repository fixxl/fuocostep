/*
 * terminal.c
 *
 *  Created on: 15.06.2013
 *      Author: Felix
 */

#include "global.h"

void terminal_reset( void ) {
    uart_puts_P( PSTR( "\033[2J" ) );
    uart_puts_P( PSTR( "\033[0;0H" ) );
    uart_puts_P( PSTR( "\033[3J" ) );
    uart_puts_P( PSTR( "\033]0;FUOCO STEP\007" ) );
}

// Print number and put spaces in front to achieve a defined length
void fixedspace( int32_t zahl, uint8_t type, uint8_t space ) {
    uint8_t cntr     = 0;
    int32_t num_temp = zahl;

    while ( num_temp ) {
        num_temp /= 10;
        cntr++;
    }

    if ( ( zahl < 0 ) && space ) {
        space--;
    }

    if ( space >= cntr ) {
        space -= cntr;

        for ( uint8_t i = space; i; i-- ) {
            uart_puts_P( PSTR( " " ) );
        }
    }

    uart_shownum( zahl, type );
}

// GUI-Routine to change IDs (allows numbers from 01 to 30)
static uint8_t enternumber( void ) {
    uint16_t number   = 0;
    uint8_t  num      = 0;
    char     entry[4] = { 0 };

    for ( uint8_t i = 0; i < 3; i++ ) {
        num = uart_getc();

        if ( ( ( num >= '0' ) && ( num <= '9' ) ) || ( num == 10 ) || ( num == 13 ) ) {
            entry[i] = num > 13 ? num : 0;
        }

        if ( !entry[i] ) {
            break;
        }
        else {
            uart_putc( entry[i] );
        }
    }
    sscanf( entry, "%u", &number );

    if ( !number || ( number > 15 ) ) {
        return 0;
    }

    return ( (uint8_t)number );
}

static uint16_t entertime( void ) {
    uint8_t num;
    char    entry[10] = { 0 };

    for ( uint8_t i = 0; i < 9; i++ ) {
        num = uart_getc();

        if ( ( num | 0x20 ) == 't' ) {
            uart_putc( 'T' );
            return TRIGGERVAL;
        }

        if ( ( ( num >= '0' ) && ( num <= '9' ) ) || ( num == 10 ) || ( num == 13 ) || ( num == ':' ) || ( num == '.' ) || ( num == ',' ) ) {
            entry[i] = num > 13 ? num : 0;
        }

        if ( !entry[i] ) {
            break;
        }
        else {
            uart_putc( entry[i] );
        }
    }

    return timecalc( entry );
}

void channel_setup( uint16_t *variable_intervalls, uint16_t *fixed_intervalls, uint8_t *use_variable_intervalls ) {
    uint8_t  swpos;
    uint8_t  ivalnr   = 0;
    uint8_t  stmode   = 0;
    uint16_t ivaltime = 0;
    char     valid    = 0;
    char     darstellung[10];

    swpos = adc_read( 4 );

    uart_puts_P( PSTR( "\n\rEinstellungen der derzeitigen Schalterstellung " ) );

    if ( !swpos ) {
        uart_puts_P( PSTR( "S:\r\n" ) );
        uart_puts_P( PSTR( "Festes Intervall (nicht änderbar): Trigger" ) );
        uart_puts_P( PSTR( "\r\n\n\n\n" ) );
        swpos = 's';
    }
    else {
        uart_shownum( swpos, 'd' );
        swpos--; // Decrement because settings' index is always swpos - 1
        uart_puts_P( PSTR( ":\r\n" ) );
    }

    if ( swpos != 's' ) {
        if ( !use_variable_intervalls[swpos] ) {
            uart_puts_P( PSTR( "Festes Intervall: " ) );

            if ( fixed_intervalls[swpos] != TRIGGERVAL ) {
                timedisplay( fixed_intervalls[swpos], darstellung );
                uart_puts( darstellung );
            }
            else {
                uart_puts_P( PSTR( "Trigger" ) );
            }

            uart_puts_P( PSTR( "\r\n\n" ) );
        }
        else {
            uart_puts_P( PSTR( "Variable Intervalle:\r\n" ) );
            uart_puts_P( PSTR( " 0 (Start): Trigger\r\n" ) );
            for ( uint8_t i = 0; i < 15; i++ ) {
                fixedspace( i + 1, 'd', 2 );
                uart_puts_P( PSTR( " (" ) );
                fixedspace( i + 1, 'd', 2 );
                uart_puts_P( PSTR( "-" ) );
                fixedspace( i + 2, 'd', 2 );
                uart_puts_P( PSTR( "): " ) );

                if ( variable_intervalls[swpos * 15 + i] != TRIGGERVAL ) {
                    timedisplay( variable_intervalls[swpos * 15 + i], darstellung );
                    uart_puts( darstellung );
                }
                else {
                    uart_puts_P( PSTR( "Trigger" ) );
                }

                uart_puts_P( PSTR( "\r\n" ) );
            }
            uart_puts_P( PSTR( "\r\n\n" ) );
        }

        uart_puts_P( PSTR( "Modus verändern (M), Intervall verändern (I) oder abbrechen (andere Taste)? " ) );

        while ( !valid )
            valid = uart_getc();

        uart_putc( valid );
        uart_puts_P( PSTR( "\r\n" ) );

        switch ( valid | 0x20 ) {
            case 'i': {
                if ( use_variable_intervalls[swpos] ) {
                    uart_puts_P( PSTR( "\r\nIntervallnummer (1-15, 0 nicht änderbar): " ) );
                    ivalnr = enternumber();
                }

                if ( ivalnr-- || !use_variable_intervalls[swpos] ) {
                    uart_puts_P( PSTR( "\r\nNeuer Wert (Step: 0:00.00 - 9:59.99, t für getriggerten Betrieb): " ) );
                    ivaltime = entertime();

                    if ( ivaltime != RETURNERROR ) {
                        uart_puts_P( PSTR( "\r\nSetze Intervall auf: " ) );

                        if ( ivaltime != TRIGGERVAL ) {
                            timedisplay( ivaltime, darstellung );
                            uart_puts( darstellung );
                        }
                        else {
                            uart_puts_P( PSTR( "Trigger" ) );
                        }

                        uart_puts_P( PSTR( "\r\n\n" ) );

                        if ( use_variable_intervalls[swpos] ) {
                            variable_intervalls[swpos * 15 + ivalnr] = ivaltime;
                            eewrite16( variable_intervalls[swpos * 15 + ivalnr], 35 + 30 * swpos + 2 * ivalnr );
                        }
                        else {
                            fixed_intervalls[swpos] = ivaltime;
                            eewrite16( fixed_intervalls[swpos], 13 + 2 * swpos );
                        }
                    }
                }

                break;
            }
            case 'm': {
                uart_puts_P( PSTR( "\r\nSteppinterval (f)est oder (v)ariabel: " ) );
                while ( !stmode ) stmode = uart_getc();
                uart_putc( stmode );
                uart_puts_P( PSTR( "\r\n" ) );
                ivalnr = use_variable_intervalls[swpos];

                if ( ( stmode | 0x20 ) == 'f' ) {
                    use_variable_intervalls[swpos] = 0;
                }

                if ( ( stmode | 0x20 ) == 'v' ) {
                    use_variable_intervalls[swpos] = 1;
                }

                if ( ivalnr != use_variable_intervalls[swpos] ) {
                    eewrite( use_variable_intervalls[swpos], swpos + 2 );
                }

                break;
            }
            default: {
                break;
            }
        }
        uart_puts_P( PSTR( "\r\n" ) );
    }
}

// List ignition devices
void list_complete( uint16_t *variable_intervalls, uint16_t *fixed_intervalls, uint8_t *use_variable_intervalls ) {
    char    darstellung[10];
    uint8_t swpos;

    terminal_reset();
    uart_puts_P( PSTR( "\n\rAktuelle Einstellungen\n\r" ) );
    uart_puts_P( PSTR( "======================\n\r" ) );
    uart_puts_P( PSTR( "Derzeitige Schalterposition: " ) );
    swpos = adc_read( 4 );

    if ( swpos ) {
        uart_shownum( swpos, 'd' );
    }
    else {
        uart_puts_P( PSTR( "S" ) );
    }

    uart_puts_P( PSTR( "\r\n\nSchalterpos. Fest/Variabel\n\r" ) );

    if ( !swpos ) {
        uart_puts( TERM_COL_YELLOW );
    }
    else {
        uart_puts( TERM_COL_WHITE );
    }

    uart_puts_P( PSTR( " S           Fixed: Trigger\r\n\n" ) );

    for ( uint8_t i = 0; i < 11; i++ ) {
        if ( i == swpos - 1 ) {
            uart_puts( TERM_COL_YELLOW );
        }
        else {
            uart_puts( TERM_COL_WHITE );
        }

        fixedspace( i + 1, 'd', 2 );
        uart_puts_P( PSTR( "           " ) );
        use_variable_intervalls[i] ? uart_puts_P( PSTR( "Variable\r\n             " ) ) : uart_puts_P( PSTR( "Fixed: " ) );

        if ( use_variable_intervalls[i] ) {
            for ( uint8_t j = 0; j < 15; j++ ) {
                if ( variable_intervalls[i * 15 + j] == TRIGGERVAL ) {
                    uart_puts_P( PSTR( "Trigger" ) );
                }
                else {
                    timedisplay( variable_intervalls[i * 15 + j], darstellung );
                    uart_puts( darstellung );
                }

                if ( j != 14 ) {
                    uart_puts_P( PSTR( ", " ) );
                }
                else {
                    uart_puts_P( PSTR( "\r\n\n" ) );
                }
            }
        }
        else {
            if ( fixed_intervalls[i] != TRIGGERVAL ) {
                timedisplay( fixed_intervalls[i], darstellung );
                uart_puts( darstellung );
            }
            else {
                uart_puts_P( PSTR( "Trigger" ) );
            }

            uart_puts_P( PSTR( "\r\n\n" ) );
        }
    }
}

uint8_t igniter_setup( uint8_t ignition_setting ) {
    uint8_t valid = 0;

    uart_puts_P( PSTR( "\r\nDerzeit eingestelltes Anzündmittel\r\n==================================\r\n\n" ) );

    if ( ignition_setting == 200 ) {
        uart_puts_P( PSTR( "TALON (2 s)" ) );
    }
    else {
        uart_puts_P( PSTR( "E-ANZÜNDER (20 ms)" ) );
    }

    uart_puts_P( PSTR( "\r\nAuswahl: (E)-Anzünder, (T)alon, Abbruch mit anderer Taste: " ) );
    while ( !valid )
        valid = uart_getc();

    uart_putc( valid );
    uart_puts_P( PSTR( "\r\n" ) );

    switch ( valid | 0x20 ) {
        case 'e': {
            return 2;
        }
        case 't': {
            return 200;
        }
        default: {
            return ignition_setting;
        }
    }
}