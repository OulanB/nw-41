/*
 *--------------------------------------
 * Program Name: hp41c
 * Author: O2S
 * License: free
 * Description: lcd display emulator
 *--------------------------------------
*/

#include <string.h>     // for mem...

#include "gfx.h"

#include "main.h"

#include "../../api/extapp_api.h"

#define LCDY 180

uint8_t lcdEnable;

static uint8_t lA[12];
static uint8_t lB[12];
static uint8_t lC[12];
uint16_t lE;

static uint8_t changedL = 0;
static uint8_t changedA = 0;

const static uint16_t* gfxptr;


uint8_t lcdDigits[] = {0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B};
uint8_t lcdDots[] = {0,1,2,3,4,0,1,2,3,4,0,1};

static void zeroTm() {
    memset(lcdDigits, 32, 12);
    memset(lcdDots, 0, 12);
}

void displayOff() {
    if (lcdEnable) {
        lcdEnable = 0;
        zeroTm();
        changedA = 1;
        changedL = 1;        
    }
}

void displayOn() {
    if (lcdEnable == 0) {
        lcdEnable = 1;
        changedA = 1;
        changedL = 1;       
    }
}

void displayToggle() {
    lcdEnable = 1 - lcdEnable;
    if (lcdEnable == 0) {
        zeroTm();
    }
    changedA = 1;
    changedL = 1;        
}

void displayCi() {

}

static void readAnnun(uint8_t *c) {
    c[0] = (uint8_t) (lE & 0x00F);
    c[1] = (uint8_t) ((lE >> 4) & 0x00F);
    c[2] = (uint8_t) (lE >> 8);
    c[3] = c[4] = c[5] = c[6] = c[7] = c[8] = c[9] = c[10] = c[11] = c[12] = c[13] = 0;
}
void lcdWriteAnnun(uint8_t *c) {
    lE = (uint16_t)(c[0] | (c[1] << 4) | (c[2] << 8));
    changedA = 1;
}

static void lcdRot (uint8_t *reg, uint8_t dir) {
    int i;
    uint8_t t;
    if (dir) {
        t = reg [0];
        for(i = 0; i < 11; i++)
            reg[i] = reg[i + 1];
        reg[11] = t;
    } else {
        t = reg[11];
        for(i = 11; i > 0; i--)
            reg[i] = reg[i - 1];
        reg [0] = t;
    }
}
static void lcdRdReg12(uint8_t *nutC, uint8_t *lcd) {
    for (int i = 0, j = 11; i < 12; i++, j--) 
        nutC[i] = lcd[j];
    nutC[12] = nutC[13] = 0;
}
static void lcdRdReg1(uint8_t *c, uint8_t regs, uint8_t dir) {
    int j = 0;
    uint8_t d = (dir) ? 0 : 11;
    if ((regs & 1) != 0) {				// __a reg
        c[j++] = lA[d];
        lcdRot(lA, dir);
    }
    if ((regs & 2) != 0) {
        c[j++] = lB[d];
        lcdRot(lB, dir);
    }
    if ((regs & 4) != 0) {
        c[j++] = lC[d];
        lcdRot(lC, dir);
    }
    while (j < 14) 
        c[j++] = 0;
}
static void lcdRdReg(uint8_t *c, uint8_t regs, uint8_t n, uint8_t dir) {
    int i, j = 0;
    uint8_t d = (dir) ? 0 : 11;
    for(i = 0; i < n; i++) {
        if ((regs & 1) != 0) {				// __a reg
            c[j++] = lA[d];
            lcdRot(lA, dir);
        }
        if ((regs & 2) != 0) {
            c[j++] = lB[d];
            lcdRot(lB, dir);
        }
        if ((regs & 4) != 0) {
            c[j++] = lC[d];
            lcdRot(lC, dir);
        }
    }
    while (j < 14)
        c[j++] = 0;
}
void lcdReadReg(uint8_t adr, uint8_t *c) {
    switch (adr) {
        case 0x0:  lcdRdReg12(c, lA);  		break;
        case 0x1:  lcdRdReg12(c, lB);  		break;
        case 0x2:  lcdRdReg12(c, lC);  		break;
        case 0x3:  lcdRdReg(c, 3, 6, 0);  	break;
        case 0x4:  lcdRdReg(c, 7, 4, 0);  	break;
        case 0x5:  readAnnun(c);           	break;
        case 0x6:  lcdRdReg1(c, 4, 0); 		break;
        case 0x7:  lcdRdReg1(c, 1, 1); 		break;
        case 0x8:  lcdRdReg1(c, 2, 1); 		break;
        case 0x9:  lcdRdReg1(c, 4, 1); 		break;
        case 0xa:  lcdRdReg1(c, 1, 0); 		break;
        case 0xb:  lcdRdReg1(c, 2, 0); 		break;
        case 0xc:  lcdRdReg1(c, 3, 1); 		break;
        case 0xd:  lcdRdReg1(c, 3, 0); 		break;
        case 0xe:  lcdRdReg1(c, 7, 1); 		break;
        case 0xf:  lcdRdReg1(c, 7, 0); 		break;
    }
    if ((adr > 2) && (adr != 5)) 
        changedL = 1;
}

static void lcdWrReg(uint8_t *c, uint8_t regs, uint8_t n, uint8_t dir) {
    int i, j = 0;
    uint8_t d = (dir) ? 11 : 0;
    for (i = 0; i < n; i++) {
        if ((regs & 1) != 0) {
            lcdRot(lA, dir);
            lA[d] = c[j++];
        }
        if ((regs & 2) != 0) {
            lcdRot(lB, dir);
            lB[d] = c[j++];
        }
        if ((regs & 4) != 0) {
            lcdRot(lC, dir);
            lC[d] = c[j++] & 1;
        }
    }
}	
static void lcdWrReg12(uint8_t *nutC, uint8_t *lcd) {
    for (int i = 0; i < 12; i++)
        lcd[i] = nutC[i];
}	
static void lcdWrReg1(uint8_t *c, uint8_t regs, uint8_t dir) {
    int j = 0;
    int d = (dir) ? 11 : 0;
    if ((regs & 1) != 0) {
        lcdRot (lA, dir);
        lA[d] = c[j++];
    }
    if ((regs & 2) != 0) {
        lcdRot (lB, dir);
        lB[d] = c[j++];
    }
    if ((regs & 4) != 0) {
        lcdRot(lC, dir);
        lC[d] = c[j] & 1;
    }
}	

void lcdWriteReg(uint8_t adr, uint8_t *c) {
    switch (adr) {
        case 0x0:  lcdWrReg12(c, lA);  break;
        case 0x1:  lcdWrReg12(c, lB);  break;
        case 0x2:  lcdWrReg12(c, lC);  break;
        case 0x3:  lcdWrReg(c, 3, 6, 1);  break;
        case 0x4:  lcdWrReg(c, 7, 4, 1);  break;
        case 0x5:  lcdWrReg(c, 3, 6, 0); break;
        case 0x6:  lcdWrReg(c, 7, 4, 0); break;
        case 0x7:  lcdWrReg1(c, 1, 1);  break;
        case 0x8:  lcdWrReg1(c, 2, 1);  break;
        case 0x9:  lcdWrReg1(c, 4, 1);  break;
        case 0xa:  lcdWrReg1(c, 1, 0); break;
        case 0xb:  lcdWrReg1(c, 2, 0); break;
        case 0xc:  lcdWrReg1(c, 4, 0); break;
        case 0xd:  lcdWrReg1(c, 3, 0); break;
        case 0xe:  lcdWrReg1(c, 7, 1);  break;
        default:   lcdWrReg1(c, 7, 0); break;
    }
    changedL = 1;
}

void lcdInitialize() {
    gfxptr = (uint16_t*)(hp41gfx);
    extapp_pushRectUniform(0, LCDY, 320, 54, 0xFFFF);
    changedA = 1;
    changedL = 1;
}

static void lcdUpdateTm() {
    uint8_t car;
    for (int digit = 0, idx = 11; digit < 12; digit++, idx--) {					// 12 digits
        car = lA[digit] | ((lB[digit] & 0x03) << 4) | ((lC[digit] != 0) ? 0x40 : 0x00);
        if (car >= 80) car = 58;
        lcdDigits[idx] = car;
        car = lB[digit] >> 2;
        lcdDots[idx] = car+1;
    }
}

void lcdDrawAnnun() {
    if (lE & 0x001) extapp_drawTextSmall("ALPHA", 276, LCDY+40, 0x0000, 0xFFFF, false);
    if (lE & 0x002) extapp_drawTextSmall("PRGM", 230, LCDY+40, 0x0000, 0xFFFF, false);
    if (lE & 0x004) extapp_drawTextSmall("4", 214, LCDY+40, 0x0000, 0xFFFF, false);
    if (lE & 0x008) extapp_drawTextSmall("3", 202, LCDY+40, 0x0000, 0xFFFF, false);
    if (lE & 0x010) extapp_drawTextSmall("2", 190, LCDY+40, 0x0000, 0xFFFF, false);
    if (lE & 0x020) extapp_drawTextSmall("1", 178, LCDY+40, 0x0000, 0xFFFF, false);
    if (lE & 0x040) extapp_drawTextSmall("0", 166, LCDY+40, 0x0000, 0xFFFF, false);
    if (lE & 0x080) extapp_drawTextSmall("SHIFT", 118, LCDY+40, 0x0000, 0xFFFF, false);
    if (lE & 0x100) extapp_drawTextSmall("RAD", 86, LCDY+40, 0x0000, 0xFFFF, false);
    if (lE & 0x200) extapp_drawTextSmall("G", 78, LCDY+40, 0x0000, 0xFFFF, false);
    if (lE & 0x400) extapp_drawTextSmall("USER", 38, LCDY+40, 0x0000, 0xFFFF, false);
    if (fast) extapp_drawTextSmall("Fast", 2, LCDY+40, 0x0000, 0xFFFF, false);
//    if (lE & 0x800) extapp_drawTextSmall("BAT", 2, LCDY+40, 0x0000, 0xFFFF, false);
}

void lcdDo() {  
    if (changedL) {
        if (lcdEnable) lcdUpdateTm();
        uint16_t i = 0;
        int16_t x = 6;
        for (i = 0; i < 12; i++) {
            extapp_pushRect(x, LCDY+8, 19, 25, gfxptr + 19*25*lcdDigits[i]);
            extapp_pushRect(x+19, LCDY+13, 7, 25, gfxptr + 19*25*16*8 + 7*25*lcdDots[i]);
            x += 26;
        }
        changedL = 0;
    }
    if (changedA) {
        extapp_pushRectUniform(2, LCDY+40, 316, 10, 0xFFFF);
        if (lcdEnable) lcdDrawAnnun();
        changedA = 0;
    } 
}

uint8_t* lcdSave(uint8_t* output) {
    uint8_t* _ptr = output;
    memcpy(_ptr, lA, sizeof(lA)); _ptr += sizeof(lA);
    memcpy(_ptr, lB, sizeof(lB));  _ptr += sizeof(lB);
    memcpy(_ptr, lC, sizeof(lC));  _ptr += sizeof(lC);
    memcpy(_ptr, &lE, sizeof(lE)); _ptr += sizeof(lE);
    memcpy(_ptr, &lcdEnable, sizeof(lcdEnable)); _ptr += sizeof(lcdEnable);
    return _ptr;
}

uint8_t* lcdLoad(uint8_t* input) {
    uint8_t* _ptr = input;
    changedA = 1;
    changedL = 1;
    memcpy(lA, _ptr, sizeof(lA)); _ptr += sizeof(lA);
    memcpy(lB, _ptr, sizeof(lB));  _ptr += sizeof(lB);
    memcpy(lC, _ptr, sizeof(lC));  _ptr += sizeof(lC);
    memcpy(&lE, _ptr, sizeof(lE)); _ptr += sizeof(lE);
    memcpy(&lcdEnable, _ptr, sizeof(lcdEnable)); _ptr += sizeof(lcdEnable);
    return _ptr;
}

/*
	private static void patchLcd() {
		__page[2].patch(0, 0xC8D, 0x130);
		__page[2].patch(0, 0xC8E, 0x05F);
		__page[2].patch(0, 0xC8F, 0x0A6);
		__page[2].patch(0, 0xC90, 0x306);
		__page[2].patch(0, 0xC91, 0x0DF);
		__page[2].patch(0, 0xCAE, 0x323);
	}
*/