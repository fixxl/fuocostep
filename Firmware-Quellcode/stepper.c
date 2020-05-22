/*
 *          Firmware
 *         FUOCO STEP
 *
 *         Felix Pflaum
 *
 */

// Everything to be included is in global.h
#include "global.h"

// Global Variables
static volatile uint8_t  trigger_flag    = 0;
static volatile uint8_t  key_flag        = 0;
static volatile uint8_t  channel_monitor = 0;
static volatile uint16_t timerval        = 0;
static volatile uint16_t active_channels = 0;

#if ( COMMONINT )
    static volatile uint8_t intstate_old = 0;
#endif


void wdt_init( void )
{
    MCUSR = 0;
    wdt_disable();
    return;
}

// Initialise Key-Switch
void key_init( void )
{
    KEY_PORT |= ( 1 << KEY );
    KEY_DDR  &= ~( 1 << KEY );

    PCICR  |= ( 1 << KEYPCIE );
    KEYMSK |= ( 1 << KEY );

    OPTO_DDR &= ~( 1 << OPTO );

    // Activate Pin-Change Interrupt possibility
    PCICR |= ( 1 << OPTOPCIE );

    // Keep low-impedance path between ignition voltage and clamps closed
    MOSSWITCH_PORT &= ~( 1 << MOSSWITCH );
    MOSSWITCH_DDR  |= ( 1 << MOSSWITCH );
}

// Switch debouncing
uint8_t debounce( volatile uint8_t *port, uint8_t pin )
{
    uint8_t keystate = 0x55, ctr = 0, timer0_regb = TCCR0B;

    // Reset Timer 0
    TCNT0  = 0;
    TIFR0  = ( 1 << TOV0 );
    TCNT0  = 160;
    TCCR0B = ( 1 << CS02 | 1 << CS00 );

    // Query switch state every 10 ms until 8 identical states in a row are received
    while ( ( ( keystate != 0x00 ) && ( keystate != 0xFF ) ) || ( ctr < 8 ) ) {
        keystate <<= 1;                                // Shift left
        keystate  |= ( ( *port & ( 1 << pin ) ) > 0 ); // Write 0 or 1 to LSB

        while ( !( TIFR0 & ( 1 << TOV0 ) ) ) { // Wait for timer overflow

        }
        TIFR0 = ( 1 << TOV0 ); // Clear interrupt flag
        TCNT0 = 160;           // Preload timer

        if ( ctr < 8 ) {
            ctr++; // Make sure at least 8 queries are executed

        }
    }

    TCCR0B = timer0_regb;

    // return 1 for active switch, 0 for inactive
    return ( keystate == 0 );
}

// ------------------------------------------------------------------------------------------------------------------------

// Main programme
int main( void )
{
    wdt_disable();
    // Keep low-impedance path between ignition voltage and clamps closed
    MOSSWITCH_PORT &= ~( 1 << MOSSWITCH );
    MOSSWITCH_DDR  |= ( 1 << MOSSWITCH );

    // Local Variables
    uint16_t controlvar;
    uint8_t  temp_sreg;
    uint8_t  ignition_time;
    uint8_t  ign_time_backup;
    uint16_t scheme              = 0;
    uint16_t anti_scheme         = 0;
    uint8_t  armed               = 0;
    uint8_t  stepper_mode        = 0;
    uint8_t  fire_channel        = 1;
    uint8_t  next_channel        = 2;
    uint8_t  additional_channels = 0;


    uint16_t fixed_intervals[11]        = { 250, 500, 750, 1000, 1250, 1500, 1750, 2000, 2500, 3000, 4000 };
    uint16_t variable_intervals[165]    = { 0 };
    uint8_t  use_variable_intervals[11] = { 0 };
    uint8_t  channel_timeout[16]        = { 0 };

    for ( uint8_t i = 0; i < 11; i++ ) {
        for ( uint8_t j = 0; j < 15; j++ ) {
            variable_intervals[i * 15 + j] = fixed_intervals[i];
        }
    }

    bitfeld_t flags;
    flags.complete = 0;

    char uart_field[36] = { 0 };

    /* For security reasons the shift registers are initialised right at the beginning to guarantee a low level at the
     * gate pins of the MOSFETs and beware them from conducting directly after turning on the device.
     */
    sr_init();

    // Disable unused controller parts (2-wire-Interface, Timer 2, Analogue Comparator)
    PRR  |= ( 1 << PRTWI ) | ( 1 << PRTIM2 );
    ACSR |= 1 << ACD;

    // Initialise Timer
    timer1_init();

    // Initialise Key-Switch
    key_init();

    // Initialise LEDs
    led_init();

    // Initialise ADC and find out what kind of device the software runs on
    adc_init();

    // Initialise UART and tell the PC we're ready
    uart_init( BAUD );

    // ------------------------------------------------------------------------------

    // Initialise EEPROM
    if ( eeread( 1 ) != 0xFF ) {
        // Read setting if fixed or variable intervals and fixed interval values
        for ( uint8_t i = 0; i < 11; i++ ) {
            use_variable_intervals[i] = eeread( i + 2 );
            fixed_intervals[i]        = eeread16( 13 + 2 * i );
        }

        // Read variable intervals
        for ( uint8_t i = 0; i < 11; i++ ) {
            for ( uint8_t j = 0; j < 15; j++ ) {
                variable_intervals[i * 15 + j] = eeread16( 35 + 30 * i + 2 * j );
            }
        }
    }
    // Write standard values to EEPROM if none are stored
    else {
        for ( uint8_t i = 0; i < 11; i++ ) {
            eewrite( use_variable_intervals[i], i + 2 );
            eewrite16( fixed_intervals[i], 13 + 2 * i );
        }

        for ( uint8_t i = 0; i < 11; i++ ) {
            for ( uint8_t j = 0; j < 15; j++ ) {
                eewrite16( fixed_intervals[i], 35 + 30 * i + 2 * j );
            }
        }

        eewrite( 0x00, 1 );
        eewrite( EMATCH_TIME, 365 );
    }

    ignition_time = ( eeread( 365 ) == TALON_TIME ) ? TALON_TIME : EMATCH_TIME;

    // ------------------------------------------------------------------------------

    // Manually trigger a key event to ensure proper states
    #if (COMMONINT)
        intstate_old = ( INTERRUPT_PIN & ( ( 1 << OPTO ) | ( 1 << KEY ) ) );
    #endif
    key_flag = 1;

    // Enable Interrupts
    sei();

    // Main loop

    /*
     * Within the main loop the status of all the flags is continuously monitored and upon detection of a set flag
     * the corresponding routine is executed. Before the execution of the routine the status register is written to a local
     * variable, interrupts are disabled and the flag gets cleared, after the routine interrupts are re-enabled
     *
     */
    while ( 1 ) {
        // -------------------------------------------------------------------------------------------------------

        // Control key switch (key_flag gets set via pin-change-interrupt)
        if ( key_flag ) {
            temp_sreg = SREG;
            cli();
            key_flag = 0;

            #if DEBUGMODE
                uart_puts_P( PSTR( "Schluesselereignis\r\n" ) );
            #endif

            // Box armed: armed = 1, Box not armed: armed = 0
            armed = debounce( &KEY_PIN, KEY );

            if ( armed ) {
                #if DEBUGMODE
                    uart_puts_P( PSTR( "Kanal-Reset, Schalter einlesen, Trigger erlauben\r\n" ) );
                #endif

                fire_channel = 1;                             // Reset fire channel to the start
                stepper_mode = adc_read( 4 );                 // Read current mode setting
                OPTOMSK     |= ( 1 << OPTO );                 // Allow fire events to be triggered
                led_red_on();                                 // Signalize armed status
            }
            else {
                #if DEBUGMODE
                    uart_puts_P( PSTR( "Schalter oeffnen, Flags zuruecksetzen\r\n" ) );
                #endif

                MOSSWITCH_PORT        &= ~( 1 << MOSSWITCH ); // Block conducting path of the P-FET
                OPTOMSK               &= ~( 1 << OPTO );      // Disallow fire events to be triggered
                flags.b.fire           = 0;                   // Clear fire status
                flags.b.step           = 0;                   // Stop running step sequences
                flags.b.is_fire_active = 0;                   // Stop fire activity
                trigger_flag           = 0;                   // Unset trigger flag

                scheme      = 0;                              // Clear all active-channels
                anti_scheme = 0;                              // Clear reset scheme
                sr_shiftout( 0 );                             // Block all switches

                timer1_reset();
                timer1_off();
                led_red_off();
                led_orange_off();
                led_green_off();
            }

            SREG = temp_sreg;
        }

        // -------------------------------------------------------------------------------------------------------

        // UART-Routine
        flags.b.uart_active = ( ( UCSR0A & ( 1 << RXC0 ) ) && 1 );

        if ( flags.b.uart_active ) {
            temp_sreg = SREG;
            cli();
            flags.b.uart_active = 0;

            led_yellow_on();

            // Receive first char
            uart_field[0] = uart_getc();

            // React according to first char (ignition command or not?)
            // Ignition command is always 4 chars long
            switch ( uart_field[0] ) {
                // Any other command is received as long as it doesn't start with enter or backspace
                case 8:
                case 10:
                case 13:
                case 127: {
                    uart_field[0] = '\0';
                    break;
                }

                case 'Q': {
                    uart_putc( uart_field[0] );
                    uart_field[ 1 ] = uart_getc();
                    uart_field[ 2 ] = uart_getc();
                    if( uart_field[1] == 'w' && uart_field[2] == 161) {
                        for ( uint8_t i = 3; i < 35; i++ ) {
                            uart_field[i] = uart_getc();
                        }
                        uart_field[35] = 0;
                        flags.b.quickwrite = 1;
                    }
                    break;
                }

                default: {
                    #if !CASE_SENSITIVE
                        uart_field[0] = uart_lower_case( uart_field[0] );
                    #endif
                    uart_putc( uart_field[0] ); // Show first char so everything looks as it should

                    if ( !uart_gets( uart_field + 1 ) ) {
                        uart_puts_P( PSTR( "\033[1D" ) );
                        uart_puts_P( PSTR( " " ) );
                        uart_puts_P( PSTR( "\033[1D" ) );
                        uart_field[0] = '\0';
                    }

                    break;
                }
            }

            // "list" gives a overview over current settings
            if ( uart_strings_equal( uart_field, "list" ) ) {
                flags.b.list = 1;
            }

            // "igniter" starts the igniter selection tool
            if ( uart_strings_equal( uart_field, "igniter" ) ) {
                ign_time_backup = ignition_time;
                ignition_time   = igniter_setup( ignition_time );

                if ( ignition_time != ign_time_backup ) {
                    eewrite( ignition_time, 365 );
                }
            }

            // "edit" starts the configuration tool
            if ( uart_strings_equal( uart_field, "edit" ) ) {
                flags.b.edit = 1;
            }

            // "qr" gives current settings in machine-readable format
            if ( uart_strings_equal( uart_field, "qr" ) ) {
                flags.b.quickread = 1;
            }

            // "trigger" triggers!
            if ( uart_strings_equal( uart_field, "trigger" ) ) {
                trigger_flag = 1;
            }

            // "cls" clears terminal screen
            if ( uart_strings_equal( uart_field, "cls" ) ) {
                terminal_reset();
            }

            // "kill" resets the controller
            if ( uart_strings_equal( uart_field, "kill" ) ) {
                flags.b.reset_device = 1;
            }

            // "default" resets the stored intervals
            if ( uart_strings_equal( uart_field, "default" ) ) {
                eewrite( 0xFF, 1 );
                flags.b.reset_device = 1;
            }

            // "adccal" shows current voltage on ADC-pin
            if ( uart_strings_equal( uart_field, "adccal" ) ) {
                adc_cal( 4 );
            }

            led_yellow_off();

            if ( uart_field[0] && ( uart_field[0] != 0xFF ) ) {
                uart_puts_P( PSTR( "\n\r" ) );
            }

            SREG = temp_sreg;
        }

        // -------------------------------------------------------------------------------------------------------

        // List current settings
        if ( flags.b.list ) {
            temp_sreg = SREG;
            cli();

            flags.b.list = 0;
            stepper_mode = adc_read( 4 );
            list_complete( variable_intervals, fixed_intervals, use_variable_intervals );

            SREG = temp_sreg;
        }

        // -------------------------------------------------------------------------------------------------------

        // List current settings
        if ( flags.b.edit ) {
            temp_sreg = SREG;
            cli();

            flags.b.edit = 0;
            stepper_mode = adc_read( 4 );
            channel_setup( variable_intervals, fixed_intervals, use_variable_intervals );

            SREG = temp_sreg;
        }

        // -------------------------------------------------------------------------------------------------------

        // List current settings
        if ( flags.b.quickread ) {
            temp_sreg = SREG;
            cli();

            flags.b.quickread = 0;
            stepper_mode = adc_read( 4 );
            quickread( variable_intervals, fixed_intervals, use_variable_intervals );

            SREG = temp_sreg;
        }

        // Write new settings
        if ( flags.b.quickwrite ) {
            temp_sreg = SREG;
            cli();

            flags.b.quickwrite = 0;
            quickwrite( uart_field + 3, variable_intervals, fixed_intervals, use_variable_intervals );
        }

        // -------------------------------------------------------------------------------------------------------

        // Software-Reset via Watchdog
        if ( flags.b.reset_device ) {
            cli();

            sr_disable();

            wdt_enable( 6 );
            terminal_reset();

            while ( 1 ) {
            }
        }

        // -------------------------------------------------------------------------------------------------------

        // When device is already in a sequence, periodically check if next shot has to be triggered
        if ( flags.b.step && stepper_mode && ( fire_channel > 1 ) ) {
            // In variable intervall mode
            if ( use_variable_intervals[stepper_mode - 1] ) {
                // Check if the next channel shall be fired based on timing and the intervall value has already been reached
                if (   ( variable_intervals[( stepper_mode - 1 ) * 15 + fire_channel - 2] != TRIGGERVAL )
                   && ( timerval >= variable_intervals[( stepper_mode - 1 ) * 15 + fire_channel - 2] ) ) {

                    // Recognize simultaneously fired channels
                    next_channel        = fire_channel + 1;
                    additional_channels = 0;

                    while ( ( next_channel < 17 ) && ( !variable_intervals[( stepper_mode - 1 ) * 15 + next_channel - 2] ) ) {
                        additional_channels++;
                        next_channel++;
                    }
                    flags.b.fire = 1;
                }

                // Switch off the stepping mode if next channel is triggered by hw-trigger
                if ( variable_intervals[( stepper_mode - 1 ) * 15 + fire_channel - 2] == TRIGGERVAL ) {
                    flags.b.step = 0;
                    OPTOMSK     |= ( 1 << OPTO );                       // Stepping is over, allow triggering again
                }
            }
            // In fixed intervall mode
            else {
                // Check if the sequence shall be fired based on timing and the intervall value has already been reached
                if ( ( fixed_intervals[stepper_mode - 1] != TRIGGERVAL ) && ( timerval >= fixed_intervals[stepper_mode - 1] ) ) {

                    next_channel        = fire_channel + 1;
                    additional_channels = 0;

                    if ( !fixed_intervals[stepper_mode - 1] && ( next_channel < 17 ) ) {
                        additional_channels++;
                        next_channel++;
                    }

                    flags.b.fire = 1;
                }

                // Switch off the stepping mode if sequence is based on hw-trigger
                if ( fixed_intervals[stepper_mode - 1] == TRIGGERVAL ) {
                    flags.b.step = 0;
                    OPTOMSK     |= ( 1 << OPTO );                       // Stepping is over, allow triggering again
                }
            }
        }

        // Check if we should fire
        if ( trigger_flag ) {
            temp_sreg = SREG;
            cli();
            trigger_flag = 0; // Clear flag

            #if DEBUGMODE
                uart_puts_P( PSTR( "Trigger f. Kanal " ) );
                uart_shownum( fire_channel, 'd' );
                uart_puts_P( PSTR( "\r\n\n" ) );
            #endif

            // Check if we are not in a sequence yet
            if ( !flags.b.step ) {
                led_green_on();

                // If no key-event is pending...
                if ( !key_flag ) {
                    // Next channel will be fired immediately after trigger if we're armed
                    if ( armed ) {
                        flags.b.fire = 1;

                        // If we're not in "S"-position,
                        // activate step sequence and disallow further triggering
                        if ( stepper_mode ) {
                            flags.b.step = 1;
                            OPTOMSK     &= ~( 1 << OPTO );
                        }
                    }
                    // Nothing will be done if we're not armed
                    else{
                        led_green_off();
                    }
                }
                // If a key-event is pending, restore trigger flag for evaluation after key-event
                else {
                    trigger_flag = 1;
                }
            }

            SREG = temp_sreg;
        }

        // Fire
        if ( flags.b.fire ) {
            temp_sreg = SREG;
            cli();
            flags.b.fire = 0; // Clear flag

            if ( armed && ( fire_channel > 0 ) && ( fire_channel < 17 ) ) { // If device is armed and channel number is valid
                flags.b.is_fire_active = 1;                                 // Signalize that we're currently firing

                led_orange_on();                                            // Signalize firing
                scheme = 0;                                                 // Set mask-variable to zero

                // Calculate which channel will be the next one to fire
                // after the upcoming sequence of one or more channels
                next_channel = fire_channel + additional_channels + 1;

                // Debugging
                #if DEBUGMODE
                    uart_puts_P( PSTR( "Feuern Kanal " ) );
                    uart_shownum( fire_channel, 'd' );
                    uart_puts_P( PSTR( " und noch " ) );
                    uart_shownum( additional_channels, 'd' );
                    uart_puts_P( PSTR( " mehr." ) );
                    uart_puts_P( PSTR( "\r\n\n" ) );
                #endif

                for ( uint8_t i = 16; i; i-- ) {
                    scheme <<= 1; // Left-shift mask-variable

                    if ( i == ( fire_channel + additional_channels ) ) {
                        scheme |= 1; // Set LSB of scheme if loop variable equals number of a channel to fire

                        if ( additional_channels ) {
                            additional_channels--; // Decrement the number of additional channels

                        }
                    }
                }

                scheme |= active_channels;            // Combine newly and already active channels
                // Debugging
                #if DEBUGMODE
                    uart_puts_P( PSTR( "Schema: 0b" ) );
                    uart_shownum( scheme, 'b' );
                    uart_puts_P( PSTR( "\r\n\n" ) );
                #endif

                MOSSWITCH_PORT |= ( 1 << MOSSWITCH ); // Open or leave open P-FET-channel
                sr_shiftout( scheme );                // Write pattern to shift-register
                active_channels = scheme;             // Re-write list of currently active channels

                // (Re-)Start the interval timer
                timerval = 0;
                timer1_reset();
                timer1_on();

                // Increase channel counter
                fire_channel = next_channel;
            }

            SREG = temp_sreg;
        }

        // Check the status of active channels
        if ( flags.b.is_fire_active && channel_monitor ) { // If we're currently firing and monitoring is due
            temp_sreg = SREG;
            cli();

            channel_monitor = 0;                           // Clear the monitoring flag
            anti_scheme     = 0;                           // Reset the delete scheme

            controlvar = 1;
            for ( uint8_t i = 0; i < 16; i++ ) {
                if ( active_channels & controlvar ) { // If a given channel is currently active
                    channel_timeout[i]++;             // Increment the timeout-value for that channel

                    if ( channel_timeout[i] >= ignition_time ) { // If the channel was active for at least the ignition time
                        anti_scheme          |= controlvar;      // Set delete-bit for this channel
                        channel_timeout[i]    = 0;               // Reset channel-timeout value
                        flags.b.finish_firing = 1;               // Leave a note that a change in the list of active channels is due
                    }
                }

                controlvar <<= 1; // Left-shift mask-variable
            }
            anti_scheme ^= active_channels; // Set channels to zero, which shell be deleted AND are active, others remain active.
            SREG         = temp_sreg;
        }

        // Stop firing if it's due
        if ( flags.b.finish_firing ) {      // If we have the order to stop firing on at least one channel
            temp_sreg = SREG;
            cli();
            flags.b.finish_firing = 0;      // Mark the order as done

            // Lock respective MOSFETs
            sr_shiftout( anti_scheme );     // Perform the necessary shift-register changes
            #if DEBUGMODE
                uart_puts_P( PSTR( "Anti-Schema: 0b" ) );
                uart_shownum( anti_scheme, 'b' );
                uart_puts_P( PSTR( "\r\n\n" ) );
            #endif

            active_channels = anti_scheme;  // Re-write the list of currently active channels

            if ( !anti_scheme ) {                             // If no more channels are active at the moment
                MOSSWITCH_PORT        &= ~( 1 << MOSSWITCH ); // Block the P-FET-channel
                flags.b.is_fire_active = 0;                   // Signalize that firing is finished for now
                led_orange_off();                             // Also by turning of the "active" LED
            }

            if ( fire_channel > 16 ) {                        // If all channels are fired
                flags.b.step = 0;                             // Stop stepping

                // Turn off the interval timer
                timer1_off();
                timer1_reset();
                timerval = 0;

                fire_channel = 1;                             // Reset the channel number
            }

            if ( !flags.b.step ) {                            // If stepping is over
                led_green_off();                              // Turn off the "triggered" LED
                OPTOMSK |= ( 1 << OPTO );                     // Allow triggering again
            }

            SREG = temp_sreg;
        }
    }

    // -------------------------------------------------------------------------------------------------------

    return 0;
}

// Interrupt vectors
ISR( TIMER1_COMPA_vect ) {
    timerval++; // Raise time value representing hundredths (0.01 s)

    if ( active_channels ) {
        channel_monitor = 1; // Set a reminder to monitor the channels

    }
}

#if (COMMONINT)
    ISR( KEYINT ) {
        uint8_t intstate_new = ( INTERRUPT_PIN & ( ( 1 << OPTO ) | ( 1 << KEY ) ) );

        switch ( intstate_old ^ intstate_new ) {
            // Case 1: Toggling key, no trigger
            case ( 1 << KEY ): {
                key_flag = 1;
                break;
            }
            // Case 2: Toggling trigger, no toggling key
            case ( 1 << OPTO ): {
                // Trigger needs OPTO-PIN high and KEY-PIN low
                if ( intstate_new == ( 1 << OPTO ) ) {
                    trigger_flag = 1;
                }

                break;
            }
            // Case 3: Both toggling
            case ( ( 1 << OPTO ) | ( 1 << KEY ) ): {
                key_flag = 1;

                // Trigger needs OPTO-PIN high and KEY-PIN low
                if ( intstate_new == ( 1 << OPTO ) ) {
                    trigger_flag = 1;
                }

                break;
            }
            default: {
                break;
            }
        }

        intstate_old = intstate_new;
    }
#else
    ISR( KEYINT ) {
        key_flag = 1;
    }
    ISR( OPTOINT ) {
        if ( ( OPTOPIN & ( 1 << OPTO ) ) && !( KEYPIN & ( 1 << KEY ) ) ) {
            trigger_flag = 1;
        }
    }
#endif
