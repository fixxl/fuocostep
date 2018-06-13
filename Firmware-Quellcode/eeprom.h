/*
 * eeprom.h
 *
 * Funktionen f�r Zugriff auf eeprom gem�� ATMEGA168PA-Datenblatt
 */

#ifndef EEPROM_H_
#define EEPROM_H_

uint8_t eeread(uint16_t address);
void eewrite(uint8_t data, uint16_t address);
uint16_t eeread16(uint16_t address);
void eewrite16(uint16_t data, uint16_t address);
#endif
