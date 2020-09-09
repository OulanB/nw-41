
#ifndef NUT_H
#define NUT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

extern uint16_t stack[4];
extern uint16_t pc;
extern bool nutSleep;
extern uint8_t nutKey;
extern uint8_t ram[1024][7];

void nutXPageInit();

void nutXBankInit();
void nutMBankInit();

void nutXRomInit();
void nutMRomInit();

void nutInit();

void nutWake();

void nutRun();

uint8_t nutLoop();

void nutDumpState();

uint8_t* nutLoad(uint8_t* input);
uint8_t* nutSave(uint8_t* output);

#endif