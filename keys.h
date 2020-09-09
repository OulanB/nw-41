
#ifndef KEYS_H
#define KEYS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

void kbdDown();
void kbdUp();

// extern uint64_t key;
extern uint8_t bkey;

// uint64_t keyScan();
void keyLoop();

#endif