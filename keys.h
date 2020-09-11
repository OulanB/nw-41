
#ifndef KEYS_H
#define KEYS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

void kbdDown();
void kbdUp();

extern uint8_t bkey;

void keyLoop();

#endif
