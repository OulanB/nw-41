/*
 *--------------------------------------
 * Program Name: hp41c
 * Author: O2S
 * License: free
 * Description: lcd display emulator
 *--------------------------------------
*/

#include "gfx.h"

#include "main.h"

#include "../../api/extapp_api.h"

#define LCDY 180

uint8_t lcdEnable;

static uint8_t lA[12];
static int xA = 0;
static uint8_t lB[12];
static int xB = 0;
static uint8_t lC[12];
static int xC = 0;
uint16_t lE;

static uint8_t changedL = 0;
static uint8_t changedA = 0;

const static uint16_t* gfxptr;

uint8_t lcdDigits[] = {0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B};
uint8_t lcdDots[] = {0,1,2,3,4,0,1,2,3,4,0,1};

static inline void zeroTm() {
    uint8_t* a = lcdDigits;
    uint8_t* b = lcdDots;
    int i = 12;
    while (i--) {
        *a++ = 32;
        *b++ = 0;
    }
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

void displayCi() {      // lcd compensation, not needed ;)

}

void lcdWriteAnnun(uint8_t *c) {
    lE = (uint16_t)(c[0] | (c[1] << 4) | (c[2] << 8));
    changedA = 1;
}

void lcdReadReg(uint8_t adr, uint8_t *c) {
    int i = 0;
    switch (adr) {
        case 0x0: // l A
            for (int j = xA; i < 12; i++) {
                j = (j+11) % 12;
                c[i] = lA[j];
            } break;
        case 0x1: // l B
            for (int j = xB; i < 12; i++) {
                j = (j+11) % 12;
                c[i] = lB[j];
            } break;
        case 0x2: // l C
            for (int j = xC; i < 12; i++) {
                j = (j+11) % 12;
                c[i] = lC[j];
            } break;
        case 0x3: // l A B
            for (; i < 12; ) {
                xA = (xA+11) % 12;  
                c[i++] = lA[xA];
                xB = (xB+11) % 12; 
                c[i++] = lB[xB];
            } break;
        case 0x4: // l A B C
            for (; i < 12; ) {
                xA = (xA+11) % 12;  
                c[i++] = lA[xA];
                xB = (xB+11) % 12; 
                c[i++] = lB[xB];
                xC = (xC+11) % 12; 
                c[i++] = lC[xC];
            } break;
        case 0x5: // read annunciators
            c[i++] = (uint8_t) (lE & 0x00F);
            c[i++] = (uint8_t) ((lE >> 4) & 0x00F);
            c[i++] = (uint8_t) (lE >> 8);
            break;
        case 0x6: // 1l C
            xC = (xC+11) % 12;    
            c[i++] = lC[xC]; break;
        case 0x7: // 1r A
            c[i++] = lA[xA];
            xA = (xA+1) % 12; break;
        case 0x8: // 1r B
            c[i++] = lB[xB];
            xB = (xB+1) % 12; break;
        case 0x9: // 1r C
            c[i++] = lC[xC];
            xC = (xC+1) % 12; break;
        case 0xa: // 1l A
            xA = (xA+11) % 12;    
            c[i++] = lA[xA]; break;
        case 0xb: // 1l B
            xB = (xB+11) % 12;    
            c[i++] = lB[xB]; break;
        case 0xc: // 1r A B
            c[i++] = lA[xA];
            xA = (xA+1) % 12;
            c[i++] = lB[xB];
            xB = (xB+1) % 12; break;
        case 0xd: // 1l A B
            xA = (xA+11) % 12;    
            c[i++] = lA[xA]; 
            xB = (xB+11) % 12;    
            c[i++] = lB[xB]; break;
        case 0xe: // 1r A B C
            c[i++] = lA[xA];
            xA = (xA+1) % 12;
            c[i++] = lB[xB];
            xB = (xB+1) % 12;
            c[i++] = lC[xC];
            xC = (xC+1) % 12; break;
        default:  // 1l A B C
            xA = (xA+11) % 12;    
            c[i++] = lA[xA]; 
            xB = (xB+11) % 12;    
            c[i++] = lB[xB]; 
            xC = (xC+11) % 12;    
            c[i++] = lC[xC]; break;
    }
    while (i < 14)
        c[i++] = 0;
    if ((adr > 2) && (adr !=5))
        changedL = 1;
}

void lcdWriteReg(uint8_t adr, uint8_t *c) {
    switch (adr) {
        case 0x0:  // r A
            for (int i = 0, j = xA; i < 12;) {
                lA[j] = c[i++];
                j = (j+1) % 12;
            } break;
        case 0x1: // r B
            for (int i = 0, j = xB; i < 12;) {
                lB[j] = c[i++];
                j = (j+1) % 12;
            } break;
        case 0x2: // r C
            for (int i = 0, j = xC; i < 12;) {
                lC[j] = c[i++] & 1;
                j = (j+1) % 12;
            } break;
        case 0x3: // r A B
            for (int i = 0; i < 12;) {
                lA[xA] = c[i++];
                xA = (xA+1) % 12;
                lB[xB] = c[i++];
                xB = (xB+1) % 12;
            } break;
        case 0x4: // r A B C
            for (int i = 0; i < 12;) {
                lA[xA] = c[i++];
                xA = (xA+1) % 12;
                lB[xB] = c[i++];
                xB = (xB+1) % 12;
                lC[xC] = c[i++] & 1;
                xC = (xC+1) % 12;
            } break;
        case 0x5: // l A B
            for (int i = 0; i < 12;) {
                xA = (xA+11) % 12;        
                lA[xA] = c[i++];
                xB = (xB+11) % 12;      
                lB[xB] = c[i++];
            } break;
        case 0x6: // l A B C
            for (int i = 0; i < 12;) {
                xA = (xA+11) % 12;        
                lA[xA] = c[i++];
                xB = (xB+11) % 12;      
                lB[xB] = c[i++];
                xC = (xC+11) % 12;      
                lC[xC] = c[i++] & 1;
            } break;
        case 0x7:  // 1r A
            lA[xA] = c[0];
            xA = (xA+1) % 12; break;
        case 0x8:  // 1r B
            lB[xB] = c[0];
            xB = (xB+1) % 12; break;
        case 0x9:  // 1r C
            lC[xC] = c[0] & 1;
            xC = (xC+1) % 12; break;
        case 0xa: // 1l A
            xA = (xA+11) % 12;
            lA[xA] = c[0]; break;
        case 0xb: // 1l B
            xB = (xB+11) % 12;
            lB[xB] = c[0]; break;
        case 0xc: // 1r A B
            lA[xA] = c[0];
            xA = (xA+1) % 12;
            lB[xB] = c[1];
            xB = (xB+1) % 12; break;
        case 0xd: // 1l A B
            xA = (xA+11) % 12;
            lA[xA] = c[0];
            xB = (xB+11) % 12;
            lB[xB] = c[1]; break;
        case 0xe: // 1r A B C
            lA[xA] = c[0];
            xA = (xA+1) % 12;
            lB[xB] = c[1];
            xB = (xB+1) % 12;
            lC[xC] = c[2] & 1;
            xC = (xC+1) % 12; break;
        default:  // 1l A B C
            xA = (xA+11) % 12;
            lA[xA] = c[0];
            xB = (xB+11) % 12;
            lB[xB] = c[1];
            xC = (xC+11) % 12;
            lC[xC] = c[2] & 1; break;
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
    for (int digit = 0, idx=xA; digit < 12; digit++) {					// 12 digits
        idx = (idx+11) % 12;
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
    _ptr += copy(_ptr, lA, sizeof(lA));
    _ptr += copy(_ptr, lB, sizeof(lB));
    _ptr += copy(_ptr, lC, sizeof(lC));
    _ptr += copy(_ptr, (uint8_t*)(&lE), sizeof(lE));
    _ptr += copy(_ptr, (uint8_t*)(&lcdEnable), sizeof(lcdEnable));
    _ptr += copy(_ptr, (uint8_t*)(&xA), sizeof(xA));
    _ptr += copy(_ptr, (uint8_t*)(&xB), sizeof(xB));
    _ptr += copy(_ptr, (uint8_t*)(&xC), sizeof(xC));
    return _ptr;
}

uint8_t* lcdLoad(uint8_t* input) {
    uint8_t* _ptr = input;
    changedA = 1;
    changedL = 1;
    _ptr += copy(lA, _ptr, sizeof(lA));
    _ptr += copy(lB, _ptr, sizeof(lB));
    _ptr += copy(lC, _ptr, sizeof(lC));
    _ptr += copy((uint8_t*)(&lE), _ptr, sizeof(lE));
    _ptr += copy((uint8_t*)(&lcdEnable), _ptr, sizeof(lcdEnable));
    _ptr += copy((uint8_t*)(&xA), _ptr, sizeof(xA));
    _ptr += copy((uint8_t*)(&xB), _ptr, sizeof(xB));
    _ptr += copy((uint8_t*)(&xC), _ptr, sizeof(xC));
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
