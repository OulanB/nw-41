/*
 *--------------------------------------
 * Program Name: hp41c
 * Author: O2S
 * License: free
 * Description: phineas timer emulator
 *--------------------------------------
*/

#include <string.h>     // for mem...

#include "main.h"

#include "nut.h"
#include "lcd.h"

#include "../../api/extapp_api.h"

static uint8_t sela = 1;				// select A or B
static uint8_t hold = 0;				// to initiate read and hold
static uint64_t clock_a = 0;			// clock reg A in ms
static uint64_t ref_a = 0;				// last access date
static uint64_t clock_b = 0;			// clock reg B in ms
static uint64_t ref_b = 0;				// android last access date
static uint64_t ref_hold = 0;
static uint8_t scratch_a[14];			// scratch reg A
static uint8_t scratch_b[14];			// scratch reg B
static uint64_t alarm_a = 0;			// alarm A      in ms
static uint64_t alarm_b = 0;		    // alarm B	    in ms
static uint8_t status[13];	            // status reg
static uint8_t afactor[4];				// accuracy factor (13 bits)

static uint32_t in_timer;				// interval timer current in ms
static uint32_t in_timer_limit = 1000;	// interval timer limit in ms
static uint64_t ref_in;					// android last access date

uint8_t phiService = 0;

#define ALMA 0
#define DTZA 1
#define ALMB 2
#define DTZB 3
#define DTZIT 4
#define PUS 5
#define CKAEN 6
#define CKBEN 7
#define ALAEN 8
#define ALBEN 9
#define ITEN 10
#define TESTA 11
#define TESTB 12

static void readClock(uint64_t clock, uint8_t* c) {
    uint64_t d = clock/10;  // in 0.01 sec in c[]
    for (int i = 0; i < 14; i++) {
        c[i] = (uint8_t)(d % 10);
        d /= 10;
    }
}

static uint64_t writeClock(uint8_t* c) {
    uint64_t clock = 0;
    for (int i = 13; i >= 0 ; i--) {
        clock += c[i];
        clock *= 10;
    }
    return clock;       // in ms
}

void phiReadReg(uint8_t adr, uint8_t *c) {
    switch (adr) {
        case 0x0:						// read clock
            if (sela) {
                uint64_t ref = extapp_millis();
                if (status[CKAEN]) {
                    clock_a += ref - ref_a;
                    ref_a = ref;
                }
                readClock(clock_a, c);
            } else {
                uint64_t ref = extapp_millis();
                if (status[CKBEN]) {
                    clock_b += ref - ref_b;
                    ref_b = ref;
                }
                readClock(clock_b, c);
            }
            break;
        case 0x1:						// read and hold    
            hold = 1;
            if (sela) {
                if (status[CKAEN]) {
                    uint64_t ref = extapp_millis();
                    clock_a += ref - ref_a;
                    ref_a = ref;
                    ref_hold = ref_a;
                } else ref_hold = 0;
                readClock(clock_a, c);
            } else {
                if (status[CKBEN]) {
                    uint64_t ref = extapp_millis();
                    clock_b += ref - ref_b;
                    ref_b = ref;
                    ref_hold = ref_b;
                } else ref_hold = 0;
                readClock(clock_b, c);
            }
            break;
        case 0x2:    					// read alarm
            readClock(sela ? alarm_a : alarm_b, c);
            break;
        case 0x3:
            if (sela) {				// read status
                c[0] = status[0] | (status[1] << 1) | (status[2] << 2) | (status[3] << 3);
                c[1] = status[4] | (status[5] << 1) | (status[6] << 2) | (status[7] << 3);
                c[2] = status[8] | (status[9] << 1) | (status[10] << 2) | (status[11] << 3);
                c[3] = status[12];
                c[4] = c[5] = c[6] = c[7] = c[8] = c[9] = c[10] = c[11] = c[12] = c[13] = 0;
            } else {					// read accuracy factor
                c[0] = 0;
                c[1] = afactor[0];
                c[2] = afactor[1];
                c[3] = afactor[2];
                c[4] = afactor[3];
                c[5] = c[6] = c[7] = c[8] = c[9] = c[10] = c[11] = c[12] = c[13] = 0;
            }
            break;
        case 0x4:						// read scratch reg
            memcpy(c, sela ? scratch_a : scratch_b, 14);
            break;
        case 0x5:  						// read interval timer
            if (status[ITEN]) {
                uint64_t ref = extapp_millis();
                in_timer += (uint32_t) (ref - ref_in);
                if (in_timer >= in_timer_limit) {
                    // in_timer %= in_timer_limit; 
                }
                ref_in = ref;
            }
            readClock((uint64_t)(in_timer), c);
            break;
        case 0x6:  break;
        case 0x7:  break;
        case 0x8:  break;
        case 0x9:  break;
        case 0xa:  break;
        case 0xb:  break;
        case 0xc:  break;
        case 0xd:  break;
        case 0xe:  break;
        default:  break;
    }
}

void phiWriteReg(uint8_t adr, uint8_t *c) {
    switch (adr) {
        case 0x0: 						// write clock
            if (sela) {
                clock_a = writeClock(c);
            } else {
                clock_b = writeClock(c);
            }
            break;				
        case 0x1:    					// write and correct
            if (sela) {
                clock_a = writeClock(c);
                if (status[CKAEN]) {
                    uint64_t ref = extapp_millis();
                    clock_a += ref - ref_hold;
                }
            } else {
                clock_b = writeClock(c);
                if (status[CKBEN]) {
                    uint64_t ref = extapp_millis();
                    clock_b += ref - ref_hold;
                }
            }
            if (hold) {				// do the correct
                hold = 0;
            }
            break;
        case 0x2:    					// write alarm
            if (sela) {
                alarm_a = writeClock(c);
            } else {
                alarm_b = writeClock(c);
            }
            break;
        case 0x3:    					
            if (sela) {				// write status
                if ((c[0] & 0x1) == 0)
                    status[ALMA] = 0;
                if ((c[0] & 0x2) == 0)
                    status[DTZA] = 0;
                if ((c[0] & 0x4) == 0)
                    status[ALMB] = 0;
                if ((c[0] & 0x8) == 0)
                    status[DTZB] = 0;
                if ((c[1] & 0x1) == 0)
                    status[DTZIT] = 0;
                if ((c[1] & 0x2) == 0)
                    status[PUS] = 0;
                phiService = status[ALMA] | status[ALMB] | status[DTZA] | status[DTZB] | status[DTZIT];
            } else {					// write accuracy factor
                afactor[0] = c[1];
                afactor[1] = c[2];
                afactor[2] = c[3];
                afactor[3] = c[4] & 0x1;
            }
            break;
        case 0x4:    					// write scratch
            memcpy(sela ? scratch_a : scratch_b, c, 14);
            break;
        case 0x5:  						// write interval timer and start
            ref_in = extapp_millis();
            in_timer = 0;
            in_timer_limit = (uint32_t) ((c[0]+10*(c[1]+10*(c[2]+10*(c[3]+10*c[5]))))*10);
            if (in_timer_limit < 100)
                in_timer_limit = 100;
            // printDec8XY(in_timer_limit, 2, 84);
            status[ITEN] = 1;
            break;
        case 0x6: break;				// nothing
        case 0x7:						// stop interval timer 
            // printDec8XY(0, 2, 84);
            status[ITEN] = 0;
            break;
        case 0x8:						// clear test mode   
            break;
        case 0x9:   					// set test mode
            break;
        case 0xa:    					// disable alarm
            status[sela ? ALAEN : ALBEN] = 0;
            break;
        case 0xb:   					// enable alarm
            status[sela ? ALAEN : ALBEN] = 1;
            break;
        case 0xc:   					// stop clock
            if (sela) {
                if (status[CKAEN]) {
                    uint64_t ref = extapp_millis();
                    clock_a += ref - ref_a;
                    ref_a = ref;
                }
                status[CKAEN] = 0;
            } else {
                if (status[CKBEN]) {
                    uint64_t ref = extapp_millis();
                    clock_a += ref - ref_b;
                    ref_b = ref;
                }
                status[CKBEN] = 0;
            }
            break;
        case 0xd:						// start clock    
            if (sela) {
                if (!status[CKAEN]) ref_a = extapp_millis();
                status[CKAEN] = 1;
            } else {
                if (!status[CKBEN]) ref_b = extapp_millis();
                status[CKBEN] = 1;
            }
            break;
        case 0xe:   
            sela = 0;		// select B
            break;
        default: 
            sela = 1;		// select A
            break;
    }
}

void phiInitialize() {
    sela = 0;
    hold = 0;
    clock_a = 0;
    ref_a = extapp_millis();
    clock_b = 0;
    ref_b = ref_a;
    ref_hold = ref_a;
    memset(scratch_a, 0, 14);
    memset(scratch_b, 0, 14);
    alarm_a = 0;
    alarm_b = 0;
    memset(status, 0, 13);
    phiService = 0;
}

void phiLoop() {    // called at 20 Hz ...
    uint64_t ref = extapp_millis();
    if (status[CKAEN]) {
        clock_a += ref - ref_a;
        ref_a = ref;
        if (status[ALAEN]) {
            if (clock_a >= alarm_a) {
                if (!status[ALMA]) {
                    status[ALMA] = 1;
                    phiService = 1;
                    nutWake(lcdEnable ? 0 : 1);
                    alarm_a = 0xFFFFFFFFFFFFFFFF;
                }
            }
        }
    }
    if (status[CKBEN]) {
        clock_b += ref - ref_b;
        ref_b = ref;
        if (status[ALBEN]) {
            if (clock_b >= alarm_b) {
                if (!status[ALMB]) {
                    status[ALMB] = 1;
                    phiService = 1;
                    nutWake();
                    status[ALBEN] = 0;
                    alarm_b = 0xFFFFFFFFFFFFFFFF;
                }
            }
        }
    }
    if (status[ITEN]) {
        in_timer += (uint32_t) (ref - ref_in);
        ref_in = ref;
        if (in_timer >= in_timer_limit) {
            if (!status[DTZIT]) {
                status[DTZIT] = 1;
                phiService = 1;
                nutWake();
                in_timer = 0;
                // printDec8XY(1, 2, 84);
            }
        }
    }
}

uint8_t* phiSave(uint8_t* output) {
    uint8_t* _ptr = output;
    uint64_t ref = extapp_millis();
    if (status[CKAEN]) {
        clock_a += ref - ref_a; ref_a = ref;
    }
    if (status[CKBEN]) {
        clock_b += ref - ref_b; ref_b = ref;
    }
    if (status[ITEN]) {
        in_timer += (uint32_t) (ref - ref_in); ref_in = ref;    
    }
    memcpy(_ptr, &sela, sizeof(sela));  _ptr += sizeof(sela);
    memcpy(_ptr, &hold, sizeof(hold));  _ptr += sizeof(hold);
    memcpy(_ptr, &clock_a, sizeof(clock_a)); _ptr += sizeof(clock_a);
    memcpy(_ptr, &ref_a, sizeof(ref_a)); _ptr += sizeof(ref_a);
    memcpy(_ptr, &clock_b, sizeof(clock_b)); _ptr += sizeof(clock_b);
    memcpy(_ptr, &ref_b, sizeof(ref_b)); _ptr += sizeof(ref_b);
    memcpy(_ptr, &ref_hold, sizeof(ref_hold));  _ptr += sizeof(ref_hold);
    memcpy(_ptr, &scratch_a, sizeof(scratch_a)); _ptr += sizeof(scratch_a);
    memcpy(_ptr, &scratch_b, sizeof(scratch_b)); _ptr += sizeof(scratch_b);
    memcpy(_ptr, &alarm_a, sizeof(alarm_a)); _ptr += sizeof(alarm_a);
    memcpy(_ptr, &alarm_b, sizeof(alarm_b)); _ptr += sizeof(alarm_b);
    memcpy(_ptr, &status, sizeof(status)); _ptr += sizeof(status);
    memcpy(_ptr, &afactor, sizeof(afactor)); _ptr += sizeof(afactor);
    memcpy(_ptr, &phiService, sizeof(phiService)); _ptr += sizeof(phiService);
    return _ptr;
}

uint8_t* phiLoad(uint8_t* input) {
    uint8_t* _ptr = input;
    uint64_t ref = extapp_millis();
    memcpy(&sela, _ptr, sizeof(sela));  _ptr += sizeof(sela);
    memcpy(&hold, _ptr, sizeof(hold));  _ptr += sizeof(hold);
    memcpy(&clock_a, _ptr, sizeof(clock_a)); _ptr += sizeof(clock_a);
    memcpy(&ref_a, _ptr, sizeof(ref_a)); _ptr += sizeof(ref_a);
    memcpy(&clock_b, _ptr, sizeof(clock_b)); _ptr += sizeof(clock_b);
    memcpy(&ref_b, _ptr, sizeof(ref_b)); _ptr += sizeof(ref_b);
    memcpy(&ref_hold, _ptr, sizeof(ref_hold));  _ptr += sizeof(ref_hold);
    memcpy(&scratch_a, _ptr, sizeof(scratch_a)); _ptr += sizeof(scratch_a);
    memcpy(&scratch_b, _ptr, sizeof(scratch_b)); _ptr += sizeof(scratch_b);
    memcpy(&alarm_a, _ptr, sizeof(alarm_a)); _ptr += sizeof(alarm_a);
    memcpy(&alarm_b, _ptr, sizeof(alarm_b)); _ptr += sizeof(alarm_b);
    memcpy(&status, _ptr, sizeof(status)); _ptr += sizeof(status);
    memcpy(&afactor, _ptr, sizeof(afactor)); _ptr += sizeof(afactor);
    memcpy(&phiService, _ptr, sizeof(phiService)); _ptr += sizeof(phiService);
    if (status[CKAEN]) {
        clock_a += ref - ref_a; ref_a = ref;
    }
    if (status[CKBEN]) {
        clock_b += ref - ref_b; ref_b = ref;
    }
    if (status[ITEN]) {
        in_timer += (uint32_t) (ref - ref_in); ref_in = ref;    
    }
    return _ptr;
}
