/*
 *--------------------------------------
 * Program Name: hp41c
 * Author: O2S
 * License: free
 * Description: scanner key emulator
 *--------------------------------------
*/

#include "../../api/extapp_api.h"

#include "nut.h"
#include "lcd.h"

#define K_WAIT 0
#define K_IDLE 1
#define K_PRESSED 2
#define K_RELEASED 3
#define K_WAIT_CHECK 4

static uint8_t keyDown = 0;
static uint8_t keyCounter = 0;

uint8_t keyState = K_WAIT;					
static uint8_t mscOn = 0;

void kbdPressKey(uint8_t key) {
    if ((nutSleep && !lcdEnable && (key == 0x18)) || (!nutSleep || lcdEnable)) {
        // extapp_drawTextSmall("key down", 2, 32, 0xFFFF, 0x0000, false);
        nutKey = key;
        keyDown = 1;
        if (keyState == K_IDLE) {
            keyState = K_PRESSED;
        }
    }
}

void kbdReleaseKey() {
    keyDown = 0;
    if (keyState == K_PRESSED) {
        // extapp_drawTextSmall("key up  ", 2, 32, 0xFFFF, 0x0000, false);
        keyState = K_RELEASED;
////        if (Key.__onKey._pressed && Key.__backKey._pressed) {		// for master clear
////            __key_state = __K_STATE.WAIT;								// but do not wake the scanner
////            __msc_on = true;											// master clear on
////            __key_counter = 4;											// prepare for 4 checks
////        }
    }
} 

uint8_t scannerOpCheckKeyboard() {
    uint8_t kyf = (keyState == K_PRESSED) || (keyState == K_RELEASED);
    if (keyState == K_WAIT_CHECK) {
        keyState = K_WAIT;
        keyCounter = 16;
    }
    if (mscOn) {													// master clear on
        keyCounter--;												
        if (keyCounter == 0) {										// wait for 4 checks
            mscOn = 0;												// master clear is done
            keyDown = 1;
            keyState = K_PRESSED; 
        }
    }
    return (kyf) ? 1 : 0;
}

void scannerOpResetKeyboard() {
    if (keyState == K_RELEASED) {
        keyState = K_WAIT_CHECK;
    }
}

void scannerLoop() {
    if (nutSleep) {
        if (keyState == K_RELEASED)
            keyState = K_IDLE;
        if ((keyState == K_PRESSED)) { // && !Nut.__wake) {
            // if (nutKey == 0x18)
                nutWake();
        }
        if ((keyState == K_WAIT)) { // && !Nut.__wake) {
            if (keyDown) {
                keyState = K_PRESSED;
                // if (nutKey == 0x18)
                    nutWake();
            } else 
                keyState = K_IDLE;
        } 
    } else {
        if (keyState == K_WAIT) {
            if (keyCounter == 0) {
                if (keyDown)
                    keyState = K_PRESSED;
                else
                    keyState = K_IDLE;
            } else
                keyCounter--;
        }
    }
}