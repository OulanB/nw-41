#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>

extern uint8_t fast;

void printHex2XY(uint8_t d, int16_t x, int16_t y);
void printHex4XY(uint16_t d, int16_t x, int16_t y);
void printHex6XY(uint32_t d, int16_t x, int16_t y);
void printHex8XY(uint32_t d, int16_t x, int16_t y);
void printHex16XY(uint64_t d, int16_t x, int16_t y);

void printDec8XY(uint32_t d, int16_t x, int16_t y);
void printDec16XY(uint64_t d, int16_t x, int16_t y);

void printRegXY(uint8_t* c, int16_t x, int16_t y);
void printRamXY(uint8_t* c, int16_t x, int16_t y);

int copy(uint8_t* dest, uint8_t* src, int len);
void zero(uint8_t* dest, int len);

#endif
