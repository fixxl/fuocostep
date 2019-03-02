/*
 * testit.c
 *
 *  Created on: 15.06.2013
 *      Author: Felix
 */

#include "global.h"

#define RETURNERROR 0xFFFF

uint16_t timecalc(char* timestring) {
    uint8_t len, cols = 0, dots = 0, form = 0, dotpos = 20;
    uint16_t min = 0, sec = 0, hun = 0;
    uint16_t timerval;

    timestring[10] = 0;

    len = strlen(timestring);
    if (len > 8) len = 8;
    if (!len) return RETURNERROR;

    for (uint8_t i = 0; i < len; i++) {
        if(timestring[i] == ',') timestring[i] = '.';

        if(timestring[i] == ':') cols++;
        if(timestring[i] == '.') {
            dotpos = i;
            dots++;
        }

        if(!((timestring[i] != 47) && (timestring[i] > 45) && (timestring[i] < 59)) || (cols > 1) || (dots > 1)) {
            uart_puts_P(PSTR("Invalid char in timestring\n"));
            return RETURNERROR;
        }
    }

    if(dots && timestring[len-2] == '.') {
        timestring[len] = '0';
        timestring[len + 1] = 0;
    }

    if(dots) timestring[dotpos + 3] = 0;

    cols <<= 1;

    form = cols + dots;

    if (timestring[0] == ':') form |= 4;
    if (timestring[0] == '.') form |= 8;

    switch(form & 0x03) {
        case 3: {
            if(form & 0x04) sscanf(timestring, ":%u.%u", &sec, &hun);
            else sscanf(timestring, "%u:%u.%u", &min, &sec, &hun);
            break;
        }
        case 2: {
            sscanf(timestring, "%u:%u", &min, &sec);
            hun = 0;
            break;
        }
        case 1: {
            if(form & 0x08) sscanf(timestring, ".%u", &hun);
            else sscanf(timestring, "%u.%u", &sec, &hun);
            min = 0;
            break;
        }
        default: {
            sscanf(timestring, "%u", &sec);
            min = 0;
            hun = 0;
            break;
        }
    }

    if(min > 9) return TRIGGERVAL;

    if((min && (sec > 59)) || (hun > 99) ) {
        uart_puts_P(PSTR("Invalid Format"));
        return RETURNERROR;
    }

    if (sec > 59) {
        min = sec/60;
        sec %= 60;
    }

    timerval = min*6000 + sec * 100 + hun;
    // uart_puts("min = %u, sec = %u, hun = %u --> timerval = %u", min, sec, hun, timerval);

    return timerval;
}

uint8_t timedisplay(uint16_t timerval, char *output_string) {
    uint16_t min = 0, sec = 0, hun = 0;

    if(timerval > 59999) return 1;

    hun = timerval % 100;
    min = timerval / 6000;
    sec = (timerval - min*6000)/100;

    sprintf(output_string, "%02u:%02u.%02u", min, sec, hun);

    return 0;
}
