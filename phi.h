
#ifndef PHI_H
#define PHI_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

extern uint8_t phiService;

void phiReadReg(uint8_t adr, uint8_t *c);
void phiWriteReg(uint8_t adr, uint8_t *c);

void phiInitialize();
void phiLoop();

uint8_t* phiLoad(uint8_t* input);
uint8_t* phiSave(uint8_t* output);

#endif