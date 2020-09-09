
#ifndef LCD_H
#define LCD_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

extern uint8_t lcdEnable;

extern uint16_t lE;

void displayOff();
void displayOn();
void displayToggle();
void displayCi();

void lcdWriteAnnun(uint8_t *c);
void lcdReadReg(uint8_t adr, uint8_t *c);
void lcdWriteReg(uint8_t adr, uint8_t *c);

void lcdInitialize();
void lcdUpdateTm();
void lcdDrawLcd();
void lcdDo();

uint8_t* lcdLoad(uint8_t* input);
uint8_t* lcdSave(uint8_t* output);

#endif