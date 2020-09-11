/*
 *--------------------------------------
 * Program Name: HP41
 * Author: O2S
 * License: free
 * Description: HP41 emulator
 *--------------------------------------
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "nut.h"
#include "lcd.h"
#include "scanner.h"
#include "keys.h"
#include "phi.h"

#include "../../api/extapp_api.h"

// #define DEBUG

int copy(uint8_t* dest, uint8_t* src, int len) {
    int l = len;
    while (len--) {
        *dest++ = *src++;
    }
    return l;
}
void zero(uint8_t* dest, int len) {
    while (len--) {
        *dest++ = 0;
    }
}

void gfxInitialize() {
    extapp_pushRectUniform(0, 0, 320, 240, 0x0000); // light gray screen
#ifdef DEBUG
    extapp_drawTextSmall("key:", 2, 24, 0xFFFF, 0x0000, false);
#endif
    extapp_drawTextSmall("    ", 2, 220, 0x0000, 0xFFFF, false);
}

const char hex[] = "0123456789ABCDEF";
char buf[] = "012345678901234567890xx";


void printHex2XY(uint8_t d, int16_t x, int16_t y) {
    buf[0] = hex[d >> 4];
    buf[1] = hex[d & 0xF];
    buf[2] = 0x00;
    extapp_drawTextSmall(buf, x, y, 0xFFFF, 0x0000, false);
}
void printHex4XY(uint16_t d, int16_t x, int16_t y) {
    buf[0] = hex[d >> 12];
    buf[1] = hex[(d >> 8) & 0xF];
    buf[2] = hex[(d >> 4) & 0xF];
    buf[3] = hex[d  & 0xF];
    buf[4] = 0x00;
    extapp_drawTextSmall(buf, x, y, 0xFFFF, 0x0000, false);
}

void printHex6XY(uint32_t d, int16_t x, int16_t y) {
    buf[0] = hex[(d >> 20) & 0xF];
    buf[1] = hex[(d >> 16) & 0xF];
    buf[2] = hex[(d >> 12) & 0xF];
    buf[3] = hex[(d >>  8) & 0xF];
    buf[4] = hex[(d >>  4) & 0xF];
    buf[5] = hex[d  & 0xF];
    buf[6] = 0x00;
    extapp_drawTextSmall(buf, x, y, 0xFFFF, 0x0000, false);
}

void printHex8XY(uint32_t d, int16_t x, int16_t y) {
    buf[0] = hex[d >> 28];
    buf[1] = hex[(d >> 24) & 0xF];
    buf[2] = hex[(d >> 20) & 0xF];
    buf[3] = hex[(d >> 16) & 0xF];
    buf[4] = hex[(d >> 12) & 0xF];
    buf[5] = hex[(d >>  8) & 0xF];
    buf[6] = hex[(d >>  4) & 0xF];
    buf[7] = hex[d  & 0xF];
    buf[8] = 0x00;
    extapp_drawTextSmall(buf, x, y, 0xFFFF, 0x0000, false);
}

void printHex16XY(uint64_t d, int16_t x, int16_t y) {
    buf[0] = hex[(d >> 60) & 0xF];
    buf[1] = hex[(d >> 56) & 0xF];
    buf[2] = hex[(d >> 52) & 0xF];
    buf[3] = hex[(d >> 48) & 0xF];
    buf[4] = hex[(d >> 44) & 0xF];
    buf[5] = hex[(d >> 40) & 0xF];
    buf[6] = hex[(d >> 36) & 0xF];
    buf[7] = hex[(d >> 32) & 0xF];
    buf[8] = hex[(d >> 28) & 0xF];
    buf[9] = hex[(d >> 24) & 0xF];
    buf[10]= hex[(d >> 20) & 0xF];
    buf[11]= hex[(d >> 16) & 0xF];
    buf[12]= hex[(d >> 12) & 0xF];
    buf[13]= hex[(d >>  8) & 0xF];
    buf[14]= hex[(d >>  4) & 0xF];
    buf[15]= hex[d  & 0xF];
    buf[16]= 0x00;
    extapp_drawTextSmall(buf, x, y, 0xFFFF, 0x0000, false);
}

void printDec8XY(uint32_t d, int16_t x, int16_t y) {
    int dig = 9;
    while (d != 0) {
        buf[dig--] = '0' + (d % 10);
        d /= 10;
    } 
    if (dig == 9) buf[dig--] = '0';
    while (dig >= 0) buf[dig--] = ' ';
    buf[10] = 0x00;
    extapp_drawTextSmall(buf, x, y, 0xFFFF, 0x0000, false);
}
void printDec16XY(uint64_t d, int16_t x, int16_t y) {
    int dig = 19;
    while (d != 0) {
        buf[dig--] = '0' + (d % 10);
        d /= 10;
    } 
    if (dig == 19) buf[dig--] = '0';
    while (dig >= 0) buf[dig--] = ' ';
    buf[20] = 0x00;
    extapp_drawTextSmall(buf, x, y, 0xFFFF, 0x0000, false);
}

void printRegXY(uint8_t* c, int16_t x, int16_t y){
    for (int i=0; i < 14; i++) buf[13-i] = hex[c[i]];
    buf[14] = 0x00;
    extapp_drawTextSmall(buf, x, y, 0xFFFF, 0x0000, false);
}

void printRamXY(uint8_t* c, int16_t x, int16_t y) {
    int j = 13;
    for (int i=0; i < 7; i++) {
        buf[j--] = hex[c[i] >> 4];
        buf[j--] = hex[c[i] & 0xF];
    }
    buf[14] = 0x00;
    extapp_drawTextSmall(buf, x, y, 0xFFFF, 0x0000, false);
}

static uint8_t down;
static uint8_t run;
uint8_t fast = 0;

static uint64_t startMs= 0;

static void loop(void) {
    startMs = extapp_millis();
    down = 0;
    run = 1;
    while (bkey != 6) {     // home key
        if ((bkey >= 53)) {     // no key down
            if (down) {
                down = 0;
#ifdef DEBUG
                extapp_drawTextSmall("     ", 30, 24, 0xFFFF, 0x0000, false);
#endif
                if (run) kbdUp();               // key seen by calc 
            }
        } else {
            if (down == 0) {
                down = 1;
#ifdef DEBUG
                printHex2XY(nutKey, 30, 24);
                printHex2XY(bkey, 51, 24);
#endif
                if (run) kbdDown();             // key seen by calc
                if (bkey == 5) {                // back -> Fast
                    fast = 1 - fast;
                    startMs = extapp_millis();
                    if (fast) {
                        extapp_drawTextSmall("Fast", 2, 220, 0x0000, 0xFFFF, false);
                    } else {
                        extapp_drawTextSmall("    ", 2, 220, 0x0000, 0xFFFF, false);
                    }
                }
            }
        }
        if (run) {
            if (nutLoop()) break;      // should run for 130 opcodes so 0.02 sec
            keyLoop();
            phiLoop();
            lcdDo();
            if ((!fast) || down || ((ram[14][0] & 0x02)!=0)) {   // slow or keydown or PSE in progress
                startMs += 20;
                uint64_t now = extapp_millis();
                if (startMs > now) 
                    extapp_msleep((uint32_t)(startMs - now));
            } else {
                startMs = extapp_millis();
                // extapp_msleep(1); // not full speed :(
            }
        } else {
            keyLoop();
            phiLoop();
            startMs += 20;
            uint64_t now = extapp_millis();
            if (startMs > now) 
                extapp_msleep((uint32_t)(startMs - now));
            // if (run) startMs = extapp_millis();  // when run restart
        }
    }
}

static uint8_t loadSystem() {
    if (extapp_fileExists("hp41.ram", EXTAPP_RAM_FILE_SYSTEM)) {
        size_t file_len = 0;
        uint8_t* input = (uint8_t*) (extapp_fileRead("hp41.ram", &file_len, EXTAPP_RAM_FILE_SYSTEM));
        // Handling corrupted save.
        if (file_len <= 0) goto err;
        if (input == NULL) goto err;
        uint8_t* next = nutLoad(input);
        if (next == NULL) goto err;
        next = lcdLoad(next);
        if (next == NULL) goto err;
        next = phiLoad(next);
        if (next == NULL) goto err;
        goto noerr;
    }
err:
    return 0;
noerr:  
    return 1;
}

static void saveSystem() {
    uint8_t* output = (uint8_t*) (malloc((size_t) 8000));
    if (output == NULL) goto err;
    uint8_t* next = nutSave(output);
    if (next == NULL) goto err;
    next = lcdSave(next);
    // printHex4XY((uint16_t)(next - output), 2, 72);
    // extapp_msleep(1000);
    if (next == NULL) goto err;
    next = phiSave(next);
    if (next == NULL) goto err;
    if (extapp_fileWrite("hp41.ram", (const char*) (output), (size_t)(next - output), EXTAPP_RAM_FILE_SYSTEM)) {
      goto noerr;
    }
err:
noerr:
    if (output != NULL) free(output);
}


void extapp_main() {
    gfxInitialize();
    lcdInitialize();
    phiInitialize();
    nutXPageInit();         // nothing in pages
    if (loadSystem() == 0) { 
        nutXBankInit();        // initialize X banks, deal only with curBank 
        nutMBankInit();        // initialize M banks, deal only with curBank 
        nutInit();             // initialize CPU -> MEMORY LOST
    } else {    // be carefull when adding or removing modules, code not really safe actually
        nutXBankInit();        // initialize X banks, deal only with curBank 
        nutMBankInit();        // initialize M banks, deal only with curBank 
    }
    nutXRomInit();          // put pages addresses, now deal only with bankPtr now
    nutMRomInit();          // for modules too, now deal only with bankPtr now
    loop();
    saveSystem();
}
