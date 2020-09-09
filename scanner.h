
#ifndef SCANNER_H
#define SCANNER_H

#include <stdint.h>

extern uint8_t keyState;

void kbdPressKey(uint8_t key);
void kbdReleaseKey();
uint8_t scannerOpCheckKeyboard();
void scannerOpResetKeyboard();

void scannerLoop();

#endif