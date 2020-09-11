/*
 *--------------------------------------
 * Program Name: hp41c
 * Author: O2S
 * License: free
 * Description: nut processor emulator
 *--------------------------------------
*/

#define HP41CX

// #define DEBUG

#include "main.h"

#include "lcd.h"
#include "scanner.h"
#include "keys.h"
#include "phi.h"

#ifdef HP41CX
#include "Roms/hp41xrom.h" 
#else
#include "Roms/hp41rom.h" 
#endif

#include "../../api/extapp_api.h"

#define WSIZE 14
#define WSIZE_1 13
#define WSIZE_2 12
#define EXPSIZE 3
#define EXPSIZE_1 2

bool debug = 1;

#define AUTO_OFF 6500*2*60;                        // 2 minutes
static int32_t delayLeave = AUTO_OFF;               // for AUTOOFF delay

typedef struct {
    uint8_t curBank;
    uint16_t* bankPtr[5];               // 0 : current, 1..4 for existing bank
} BANK;

static BANK bank[16];                      // all pages

#ifdef HP41CX
static uint32_t config = 0x00000001;
#else
static uint32_t config = 0x00000000;
#endif

static uint8_t rZero[14] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static uint8_t rA[14];
static uint8_t rB[14];
static uint8_t rC[14];
static uint8_t rM[14];
static uint8_t rN[14];

static uint8_t rTmp[14];
static uint8_t rG[2];

static uint8_t fFo[14];
static uint8_t fFi[14];
static uint8_t fS[14];

static uint8_t ptr, base, cy, p_cy, w_cy;
static int pp, pq;
const static uint8_t pSetMap[16] =  {3,4,5,10,8,6,11,15,2,9,7,13,1,12,0,15};

uint16_t pc;
static uint16_t ppc;            // previous pc for doBank

uint16_t stack[4];
static uint8_t gsub;

static uint16_t adr;                // current reg access address

uint8_t ram[1024][7];

const static uint8_t ramWr[1024/8] = {
#ifdef HP41CX
    0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 
    0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00
#else
    0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
#endif
};
static const uint16_t *romS;
	 
static uint8_t extSel = 0;

uint32_t dcycles = 0;
uint8_t nutSleep = 0;
static uint8_t wake = 0;

uint8_t nutKey = 0;

static inline void regCopy(uint8_t* dest, uint8_t* src, int len) {
    while (len--) {
        *dest++ = *src++;
    }
}
static inline void regExch(uint8_t* dest, uint8_t* src, int len) {
    uint8_t t;
    while (len--) {
        t = *dest; 
        *dest++ = *src;
        *src++ = t;
    }
}
static inline void regZero(uint8_t* dest, int len) {
    while (len--) {
        *dest++ = 0;
    }
}

static inline void regAdd(uint8_t* dest, uint8_t* src2,int len) {
    while (len--) {
         *dest = *dest + *src2++ + cy;
        if (*dest >= base) {
            *dest -= base;
            *dest &= 0x0F;
            cy = 1;
        } else { cy = 0; }
        dest++;
    }
}
static inline void regSub(uint8_t* dest, uint8_t* src1, uint8_t* src2, int len) {
    while(len--) {
         *dest = *src1++ - *src2++ - cy;
        if (*dest >= base) {
            *dest += base;
            *dest &= 0x0F;
            cy = 1;
        } else { cy = 0; }
        dest++;
    } 
}
static inline void regTestNE(uint8_t* src1, uint8_t* src2, int len) { 
    while (len--) {
        cy |= (*src1++ != *src2++) ? 1 : 0;
    } 
}
static inline void regShiftR(uint8_t *dest, int len) {
	while (len--) {
        *dest = (len == 0) ? 0 : dest[1];
        dest++;
    }
}
static inline void regShiftL(uint8_t *dest, int len) {
	while (len--) {
        *dest = (len == 0) ? 0 : dest[-1];
        dest--;
    } 
}
static uint16_t popStack() {
    uint16_t ret = stack[0];
    stack[0] = stack[1];
    stack[1] = stack[2];
    stack[2] = stack[3];
    return ret;
}
static void pushStack(uint16_t a) {
    stack[3] = stack[2];
    stack[2] = stack[1];
    stack[1] = stack[0];
    stack[0] = a; 
}

static void writeReg() {
    if (ramWr[adr>>3] & (1 << (adr & 0x07))) {
        ram[adr][0] = rC[0] | (rC[1] << 4);
        ram[adr][1] = rC[2] | (rC[3] << 4);
        ram[adr][2] = rC[4] | (rC[5] << 4);
        ram[adr][3] = rC[6] | (rC[7] << 4);
        ram[adr][4] = rC[8] | (rC[9] << 4);
        ram[adr][5] = rC[10] | (rC[11] << 4);
        ram[adr][6] = rC[12] | (rC[13] << 4);
    } 
}
static void readReg() {
    rC[0] = ram[adr][0] & 0x0F;
    rC[1] = ram[adr][0] >> 4;
    rC[2] = ram[adr][1] & 0x0F;
    rC[3] = ram[adr][1] >> 4;
    rC[4] = ram[adr][2] & 0x0F;
    rC[5] = ram[adr][2] >> 4;
    rC[6] = ram[adr][3] & 0x0F;
    rC[7] = ram[adr][3] >> 4;
    rC[8] = ram[adr][4] & 0x0F;
    rC[9] = ram[adr][4] >> 4;
    rC[10] = ram[adr][5] & 0x0F;
    rC[11] = ram[adr][5] >> 4;
    rC[12] = ram[adr][6] & 0x0F;
    rC[13] = ram[adr][6] >> 4;
}

static uint16_t fetchRomPc() {
    uint16_t v = 0x0000;
    int p = pc >> 12;
    int o = pc & 0x0FFF;
    if (bank[p].curBank!= 0xFF) {
        v = bank[p].bankPtr[0][o];
    }
    dcycles++;
    pc++;
    return v;
}

static uint16_t readRom(uint16_t adr) {
    uint16_t v = 0x0000;
    int p = adr >> 12;
    int o = adr & 0x0FFF;
    if (bank[p].curBank!= 0xFF) {
        v = bank[p].bankPtr[0][o];
    }
    dcycles++;
    return v;
}

static void doBankNow(int p, int b) {
    if (bank[p].curBank == 0xFF) return;
    if (bank[p].bankPtr[b] != NULL) {
        bank[p].bankPtr[0] = bank[p].bankPtr[b];
        bank[p].curBank = (uint8_t) (b);
    }
}

static void doBank(int bank) {
    int p = (ppc >> 12);
    switch(p) {
        case 0x0:
        case 0x1:
        case 0x2:
            break;
        case 0x3:
        case 0x5:
            doBankNow(3, bank);
            doBankNow(5, bank);
            break;
        case 0x4:
        case 0x6:
        case 0x7:
            doBankNow(p, bank);
            break;
        default:
            doBankNow(p & 0xE, bank);
            doBankNow((p & 0xE) + 1, bank);
            break;
    }
}

void nutXPageInit() {       // nothing in pages
    for (int p = 0; p < 16; p++) {
        bank[p].curBank = 0xFF;
        bank[p].bankPtr[0] = NULL;
        bank[p].bankPtr[1] = bank[p].bankPtr[2] = NULL;
        bank[p].bankPtr[3] = bank[p].bankPtr[4] = NULL;
    }
}

static void resetBank() {   // when POWOFF
    for (int p = 4; p < 16; p++) {
        if (bank[p].curBank != 0xFF) {
            bank[p].curBank = 1;
            bank[p].bankPtr[0] = bank[p].bankPtr[1];
        }
    }
}

static void restoreBank() {     // adjust bankPtr[0] from curBank
    for (int p = 0; p < 16; p++) {
        if (bank[p].curBank != 0xFF) {   // some rom ?
            bank[p].bankPtr[0] = bank[p].bankPtr[bank[p].curBank];
            if (bank[p].bankPtr[0] == NULL) {   // but not loaded, 
                bank[p].curBank = 0xFF;             // remove it
            }
        }
    }
}

void nutXBankInit() {       // init system banks, deal only with curBank
    bank[0].curBank = 1;
    bank[1].curBank = 1;
    bank[2].curBank = 1;
    bank[3].curBank = 1;
    bank[5].curBank = 1;
    // bank[4].curBank = 1;        // if used ...
}

void nutMBankInit() {       // init modules banks if any, deal only with curBank
    for (int p = 8; p < 16; p++) {
        if (bank[p].bankPtr[1] != NULL)
            bank[p].curBank = 1;
    }
}

void nutXRomInit() {        // system roms initialize, now deal only with bankPtr now
    uint16_t* ptr;
#ifdef HP41CX
    romS = (uint16_t *)(xnutRom);
    ptr = (uint16_t*) (romS);
    bank[0].bankPtr[1] = ptr;      // xnut0 page 0,1
    ptr += 0x1000;
    bank[1].bankPtr[1] = ptr;      // xnut1 page 1,1
    ptr += 0x1000;
    bank[2].bankPtr[1] = ptr;      // xnut2 page 2,1
    ptr += 0x1000;

    bank[3].bankPtr[1] = ptr;      // cxfun0 page 3,1
    ptr += 0x1000;
    bank[5].bankPtr[2] = ptr;      // cxfun1 page 5,2
    ptr += 0x1000;
    bank[5].bankPtr[1] = ptr;      // timer page 5,1
    ptr += 0x1000;

    // bank[4].bankPtr[1] = ptr;      // lib-4 page 4,1 ... not done
    // ptr += 0x1000;
#else
    romS = (uint16_t *)(nutRom);
    ptr = (uint16_t*) (romS);
    bank[0].bankPtr[1] = ptr;      // nut0 page 0,1
    ptr += 0x1000;
    bank[1].bankPtr[1] = ptr;      // nut1 page 1,1
    ptr += 0x1000;
    bank[2].bankPtr[1] = ptr;      // nut2 page 2,1
    ptr += 0x1000;
#endif
    restoreBank();
}

void nutMRomInit() {        // modules roms initialize, now deal only with bankPtr now
    restoreBank();
}

static void opArith(uint8_t code) {        // use code
    int first;
    int len;
    uint8_t op = (uint8_t)(code >> 3);
    switch (code & 0x07) {
        case 0:  /* pt  */
            first = (ptr) ? pp : pq;
            if (first >= WSIZE) first = 0; 
            len = 1;
            break;
        case 1:  /* x  */  
            first = 0; len = 3; 
            break;
        case 2:  /* wpt */
            first = 0; len = ((ptr) ? pp :pq)+1;
            if (len > WSIZE) len = WSIZE;
            break;
        case 3:  /* w  */  
            first = 0; len = WSIZE;  
            break;
        case 4:  /* pq */
            first = (pp >= WSIZE) ? 0 : pp;
            len = ((pp > pq) ? WSIZE_1 : ((pq >= WSIZE) ? 0 : pq)) - first + 1;
            break;
        case 5:  /* xs */  
            first = EXPSIZE_1; len = 1; 
            break;
        case 6:  /* m  */  
            first = EXPSIZE; len = 10; 
            break;
        default: /* s  */  
            first = WSIZE_1; len = 1; 
            break;
    }
    switch (op) {
        case 0x00:									  	/* A=0 */
            regZero(rA + first,len); break;
        case 0x01: 									 	/* B=0 */
            regZero(rB + first, len); break;
        case 0x02:  									/* C=0 */
            regZero(rC + first, len); break;
        case 0x03:  									/* AB EX */
            regExch(rB+first, rA+first, len); break;
        case 0x04:  									/* B=A */
            regCopy(rB+first, rA+first, len); break;
        case 0x05:  									/* AC EX */
            regExch(rC+first, rA+first, len); break;
        case 0x06:  									/* C=B */
            regCopy(rC+first, rB+first, len); break;
        case 0x07:  									/* BC EX */
            regExch(rB+first, rC+first, len); break;
        case 0x08:  									/* A=C */
            regCopy(rA+first, rC+first, len); break;
        case 0x09:  									/* A=A+B */
            regAdd(rA+first, rB+first, len); break;
        case 0x0a:  									/* A=A+C */
            regAdd(rA+first, rC+first, len); break;
        case 0x0b:  									/* A=A+1 */
            cy = 1; regAdd(rA+first, rZero, len); break;
        case 0x0c:  									/* A=A-B */
            regSub(rA+first, rA+first, rB+first, len); break;
        case 0x0d:  									/* A=A-1 */
            cy = 1; regSub(rA+first, rA+first, rZero, len); break;
        case 0x0e:  									/* A=A-C */
            regSub(rA+first, rA+first, rC+first, len); break;
        case 0x0f:  									/* C=C+C */
            regAdd(rC+first, rC+first, len); break;
        case 0x10:  									/* C=A+C */
            regAdd(rC+first, rA+first, len); break;
        case 0x11:  									/* C=C+1 */
            cy = 1; regAdd(rC+first, rZero, len); break;
        case 0x12:  									/* C=A-C */
            regSub(rC+first, rA+first, rC+first, len); break;
        case 0x13:  									/* C=C-1 */
            cy = 1; regSub(rC+first, rC+first, rZero, len); break;
        case 0x14:  									/* C=-C */
            regSub(rC+first, rZero, rC+first, len); break;
        case 0x15:  									/* C=-C-1 */
            cy = 1; regSub(rC+first, rZero, rC+first, len); break;
        case 0x16:  									/* ?B#0 */
            regTestNE(rB+first, rZero, len); break;
        case 0x17:                                      /* ?C#0 */
            regTestNE(rC+first, rZero, len); break;
        case 0x18:                                      /* ?A<C */
            regSub(rTmp, rA+first, rC+first, len); break;
        case 0x19:                                      /* ?A<B */
            regSub(rTmp, rA+first, rB+first, len); break;
        case 0x1a:                                      /* ?A#0 */
            regTestNE(rA+first, rZero, len); break;
        case 0x1b:                                      /* ?A#C */
            regTestNE(rA+first, rC+first, len); break;
        case 0x1c:                                      /* ASR */
            regShiftR(rA+first, len); break;
        case 0x1d:                                      /* BSR */
            regShiftR(rB+first, len); break;
        case 0x1e:                                      /* CSR */
            regShiftR(rC+first, len); break;
        default:                                        /* ASL */
            regShiftL(rA+first+len-1, len); break;
    }
}

static void opMisc0(uint8_t codeh) {     // use  codeh
    switch(codeh) {
        case 0x0:			// NOP
            if (gsub) pc = popStack();
            break;
        case 0x1:			// WROM for hepax ram only
            break;
        case 0x2:			// 0x080
            break;
        case 0x3:			// 0x0C0
            break;
        case 0x4:			// ENROM1
            doBank(1);
            break;
        case 0x5:			// ENROM3
            doBank(3);
            break;
        case 0x6:			// ENROM2
            doBank(2);
            break;
        case 0x7:			// ENROM4
            doBank(4);
            break;
        case 0x8:			// 0x200
            break;
        case 0x9:			// 0x240
            break;
        case 0xA:			// 0x280
            break;
        case 0xB:			// 0x2C0
            break;
        case 0xC:			// 0x300
            break;
        case 0xD:			// 0x340
            break;
        case 0xE:			// 0x380
            break;
        default:			// 0x3C0
            break;
    }
}
void opMisc6(uint8_t codeh) {
    int p; 
    uint8_t c0, c1;
    switch(codeh) {
        case 0x0:			// 0x018
            break;
        case 0x1:			// G=C
            p = (ptr) ? pp : pq;
            rG[0] = rC[p++];
            if (p == WSIZE) p = 0;
            rG[1] = rC[p]; 
            break;
        case 0x2:			// C=G
            p = (ptr) ? pp : pq;
            rC[p++] = rG[0];
            if (p == WSIZE) p = 0;
            rC[p] = rG[1]; 
            break;
        case 0x3:			// GC EX
            p = (ptr) ? pp : pq; 
            c0 = rG[0];
            rG[0] = rC[p];
            rC[p++] = c0;
            if (p == WSIZE) p = 0;            
            c1 = rG[1];
            rG[1] = rC[p];
            rC[p] = c1; 
            break;
        case 0x4:			// 0x118
            break;
        case 0x5:			// M=C
            regCopy(rM, rC, WSIZE); break;
        case 0x6:			// C=M
            regCopy(rC, rM, WSIZE); break;
        case 0x7:			// CM EX
            regExch(rM, rC, WSIZE); break;
        case 0x8:			// 0x218
            break;
        case 0x9:			// F=SB
            regCopy(fFo, fS, 8); break;
        case 0xA:			// SB=F
            regCopy(fS, fFo, 8); break;
        case 0xB:			// F EX SB      used only for TONE 
            regExch(fS, fFo, 8); break;
        case 0xC:			// 0x318
            break;
        case 0xD:			// ST=C
            fS[0] = rC[0] & 0x1;
            fS[1] = (rC[0] >> 1) & 0x1;
            fS[2] = (rC[0] >> 2) & 0x1;
            fS[3] = (rC[0] >> 3) & 0x1;
            fS[4] = rC[1] & 0x1;
            fS[5] = (rC[1] >> 1) & 0x1;
            fS[6] = (rC[1] >> 2) & 0x1;
            fS[7] = (rC[1] >> 3) & 0x1; 
            break;
        case 0xE:			// C=ST
            rC[0] = (uint8_t)(((fS[0]) ? 1 : 0) | ((fS[1]) ? 2 : 0) | ((fS[2]) ? 4 : 0) | ((fS[3]) ? 8 : 0));
            rC[1] = (uint8_t)(((fS[4]) ? 1 : 0) | ((fS[5]) ? 2 : 0) | ((fS[6]) ? 4 : 0) | ((fS[7]) ? 8 : 0));  
            break;
        default:			// CST EX
            c0 = rC[0];
            c1 = rC[1];
            rC[0] = (uint8_t)(((fS[0]) ? 1 : 0) | ((fS[1]) ? 2 : 0) | ((fS[2]) ? 4 : 0) | ((fS[3]) ? 8 : 0));
            rC[1] = (uint8_t)(((fS[4]) ? 1 : 0) | ((fS[5]) ? 2 : 0) | ((fS[6]) ? 4 : 0) | ((fS[7]) ? 8 : 0));
            fS[0] = c0 & 0x1;
            fS[1] = (c0 >> 1) & 0x1;
            fS[2] = (c0 >> 2) & 0x1;
            fS[3] = (c0 >> 3) & 0x1;
            fS[4] = c1 & 0x1;
            fS[5] = (c1 >> 1) & 0x1;
            fS[6] = (c1 >> 2) & 0x1;
            fS[7] = (c1 >> 3) & 0x1; 
            break;
    }
}
static void opMisc8(uint8_t codeh) {
    switch(codeh) {
        case 0x0:			// SPOPND
            popStack();
            break;
        case 0x1:			// POWOFF
#ifdef DEBUG
            extapp_drawTextSmall("Sleep", 2, 36, 0xFFFF, 0x0000, false);
#endif
            resetBank();
            delayLeave = AUTO_OFF;
            pc = 0x0000; nutSleep = 1; 
#ifdef DEBUG
            printHex4XY(stack[0], 2, 60);            
            printHex4XY(stack[1], 37, 60);            
            printHex4XY(stack[2], 72, 60);            
            printHex4XY(stack[3], 107, 60);            
#endif
            break;
        case 0x2:			// SELP
            ptr = 1; 
            break;
        case 0x3:			// SELQ
            ptr = 0; 
            break;
        case 0x4:			// ?P=Q
            cy = (pp == pq) ? 1 : 0; 
            break;
        case 0x5:			// ?LLD
            cy = 0;			    // never low bat :)
            break;
        case 0x6:			// CLRABC
            regZero(rA, WSIZE);
            regZero(rB, WSIZE);
            regZero(rC, WSIZE); 
            break;
        case 0x7:			// GOTOC
            pc = (rC[6] << 12) | (rC[5] << 8) | (rC[4] << 4) | rC[3];
            break;
        case 0x8:			// C=KEYS
            rC[4] = (uint8_t)(nutKey >> 4);
            rC[3] = (uint8_t)(nutKey & 0x0F);
            break;
        case 0x9:			// SETHEX
            base = 16; break;		
        case 0xA:			// SETDEC
            base = 10; break;
        case 0xB:			// DISOFF
            displayOff();
            break;
        case 0xC:			// DISTOG
            displayToggle();
            break;
        case 0xD:			// RTNC
            if (p_cy == 1) pc = popStack();
            break;
        case 0xE:			// RTNNC
            if (p_cy == 0) pc = popStack();
            break;
        default:			// RTN
            pc = popStack();
            break;
    }
    
}
static void opMiscC(uint8_t codeh) {
    uint16_t addr;
    uint16_t data;
    switch(codeh) {
        case 0x0:			// DISBLK or ROM BLK for Hepax 
            break;
        case 0x1:			// N=C
            regCopy(rN, rC, WSIZE); 
            break;
        case 0x2:			// C=N
            regCopy(rC, rN, WSIZE); 
            break;
        case 0x3:			// CN EX
            regExch(rN, rC, WSIZE); break;
        case 0x4:			// LDI
            data = fetchRomPc();
            rC[2] = (uint8_t) ((data >> 8) & 0x0F);
            rC[1] = (uint8_t) ((data >> 4) & 0xF);
            rC[0] = (uint8_t) (data & 0xF); 
            break;
        case 0x5:			// STK=C
            pushStack((rC[6] << 12) | (rC[5] << 8) | (rC[4] << 4) | rC[3]);
            break;
        case 0x6:			// C=STK
            addr = popStack();
            rC[6] = (uint8_t) (addr >> 12) & 0x0F;
            rC[5] = (uint8_t) ((addr >> 8) & 0x0F);
            rC[4] = (uint8_t) ((addr >> 4) & 0x0F);
            rC[3] = (uint8_t) (addr & 0x0F);
            break;
        case 0x7:			// 0x1F0 WPTOG for Hepax
            break;
        case 0x8:			// GOKEYS
            pc = (pc & 0xFF00) | (uint16_t)(nutKey & 0x00FF);
            break;
        case 0x9:			// DADD=C					// 3 nibles for hp41
            adr = (uint16_t)(((rC[2] & 3) << 8) | (rC[1] << 4) | rC[0]);	
            if (extSel == 2) extSel = 0; // clear phiSel
            break;
        case 0xA:			// CLRDATA not used in nut (from old cpu)
            //for (byte i,j = 0; j < 16; j++)
            //	for (i= 0; i < __WSIZE; i++)
            //		__ram[(__adr & 0xFF0) | j][i] = 0;
            break;
        case 0xB:			// DATA=C
            if (extSel & 0x01)
                lcdWriteAnnun(rC);
            else
                writeReg();
            break;
        case 0xC:			// CXISA
            data = readRom((rC[6] << 12) | (rC[5] << 8) | (rC[4] << 4) | rC[3]);
            rC[2] = (uint8_t)((data >> 8) & 0x0F);
            rC[1] = (uint8_t)((data >> 4) & 0x0F);
            rC[0] = (uint8_t)(data & 0x0F); 
            break;
        case 0xD:			// C=C|A
            for (int i = 0; i < WSIZE; i++)
                rC[i] |= rA[i]; 
            // subtle bug
            //if ((__prev_cy == 1) && (__prev_tef_last == __WSIZE_1)) {
            //	  __c[__WSIZE_1] = __c[0];
            //	  __a[__WSIZE_1] = __c[0];
            //}
            break;
        case 0xE:			// C=C&A
            for (int i = 0; i < WSIZE; i++)
                rC[i] &= rA[i]; 
            //subtle bug
            //if ((__prev_cy == 1) && (__prev_tef_last == __WSIZE_1)) {
            //	  __c[__WSIZE_1] = __c[0];
            //	  __a[__WSIZE_1] = __c[0];
            //}
            break;
        default:			// PFAD=C
            extSel =  0;
            if (rC[1] == 0xF) { 
                if (rC[0] == 0xD) {
                    extSel = 0x01;      // lcd
                } else if (rC[0] == 0xB) {
                    extSel = 0x02;      // phi
                }
            } 
            break;
    }
}
static void opMiscD(uint8_t codeh) {     // use codeh
    // for peripherals 
}

static void opMisc(uint8_t code) {      // use code 
    uint8_t codeh = (uint8_t)(code >> 4);
    switch(code & 0x0F) {
        case 0x00:					// misc 0
            opMisc0(codeh);
            break;
        case 0x01:					// Sx=0
            if (codeh == 0xF) {				// CLRST
                for (int i = 0; i <= 7; i++) fS[i] = 0;
            } else if (codeh == 0x7) {		// 0x1C4
            } else {
                fS[pSetMap[codeh]] = 0;
            }
            break;
        case 0x02:					// Sx=1
            if (codeh == 0xF) {				// RSTKB
                scannerOpResetKeyboard();
            } else if (codeh == 0x7) {		// 0x1C8
            } else {
                fS[pSetMap[codeh]] = 1;
            }
            break;
        case 0x03:					// ?Sx=1
            if (codeh == 0xF) {				// CHKKB
                cy = scannerOpCheckKeyboard();
            } else if (codeh == 0x7) {		// 0x1CC
            } else {
                cy = fS[pSetMap[codeh]];
            }
            break;
        case 0x04:					// LD constant
            if (ptr) {
                if (pp < WSIZE) rC[pp] = codeh;
                if (pp == 0) pp = WSIZE_1; else pp--;
            } else {
                if (pq < WSIZE) rC[pq] = codeh;
                if (pq == 0) pq = WSIZE_1; else pq--;
            }
            break;
        case 0x05:					// ?PT=x
            if (codeh == 0xF) {				// DECPT
                if (ptr) {
                    if (pp > 0) pp--; else pp = WSIZE_1;
                } else {
                    if (pq > 0) pq--; else pq = WSIZE_1;
                }
            } else if (codeh == 0x7) {		// 0x1D4
            } else {
                cy = (((ptr) ? pp : pq) == pSetMap[codeh]) ? 1 : 0;
            }
            break;
        case 0x06:					// misc 6
            opMisc6(codeh); 
            break;
        case 0x07:					// PT=x
            if (codeh == 0xF) {				// INCPT
                if (ptr) {
                    if (pp == WSIZE_1) pp = 0; else pp++;
                } else  {
                    if (pq == WSIZE_1) pq = 0; else pq++;
                }
            } else if (codeh == 0x7) {		// 0x1DC
            } else {						// PT=x
                if (ptr) {
                    pp = pSetMap[codeh]; 
                } else {
                    pq = pSetMap[codeh];
                }
            }
            break;
        case 0x08:					// misc 8
            opMisc8(codeh); 
            break;
        case 0x09:					// SELFP x
            break;
        case 0x0A:					// REGx=C
            adr = (adr & 0x3F0) | (uint16_t)(codeh);
            if (extSel & 0x01) {
                lcdWriteReg(codeh, rC);
            } else if (extSel & 0x02) {
                phiWriteReg(codeh, rC);
            } else {
                writeReg();
            }
            break;
        case 0x0B:					// ?Fx=1
            if (codeh == 0xF) {				// 0x1EC
            } else if (codeh == 0x7) {		// 0x3EC
            } else {					// ?Fx=1
                fFi[13] = fFi[12] = phiService;
                cy = fFi[pSetMap[codeh]];
            }
            break;
        case 0x0C:					// misc C
            opMiscC(codeh); 
            break;
        case 0x0D:					// misc D : n/a
            opMiscD(codeh); 
            break;
        case 0x0E:					// C=DATA, REGx
            if (extSel & 0x01) {
                lcdReadReg(codeh, rC);
            } else if (extSel & 0x02) {
                phiReadReg(codeh, rC);
            } else {
                if (codeh == 0) readReg();
                else {
                    adr = (uint16_t)((adr & 0x03F0) | codeh);
                    readReg();
                }
            }
            break;
        default:					// RCR x
            if (codeh == 0x7) {			// WCMD
            } else if (codeh == 0xF) {	// 0x3FC
                displayCi();
            } else {					// RCR x
                int j = pSetMap[codeh];
                for (int i = 0; i < WSIZE; i++) {
                    rTmp[i] = rC[j];
                    j = (j == WSIZE_1) ? 0 : j+1;
                }
                regCopy(rC, rTmp, WSIZE);
            }
            break;
    }
}

void nutInit() {
    int i;
    pc = 0x0000;
    cy = 0;
    nutSleep = 0;
    wake = 0;
    for (i = 0; i < WSIZE; i++) {
        rA[i] = rB[i] = rC[i] = rM[i] = rN[i];
        fS[i] = fFi[i] = 0;
    }
    rG[0] = rG[1] = 0;
    base = 16;
    ptr = 1;
    pp = 0;
    pq = 0;
    adr = 0;
    regZero(fFo, 8);
    zero((uint8_t*)(ram), 7*1024);
    displayOff();
}

void nutWake() {					// wake cpu if needed
    if (nutSleep && (wake == 0)) {
        wake = 1;
        w_cy = (lcdEnable ? 0 : 1);
    }		
}

static void evalOpcode() {
    ppc = pc;
    uint16_t opcode = fetchRomPc();
    uint16_t code = (opcode >> 2);
    cy = 0;
    switch(opcode & 0x03) {
        case 0:					// misc
            opMisc((uint8_t)(code));
            gsub = 0;
            break;
        case 1:					// branch long
            opcode = fetchRomPc();
            switch (opcode & 0x003) {
                case 0:							// GSUBNC
                    if (p_cy == 0) goto do_call;
                    goto no_branch;
                case 1:							// GSUBC
                    if (p_cy != 0) goto do_call;
                    goto no_branch;
                case 2:							// GOLNC
                    if (p_cy == 0) goto do_branch;
                    goto no_branch;
                default:						// GOLC
                    if (p_cy != 0) goto do_branch;
                    goto no_branch;
            }
do_call:
            gsub = 1;
            pushStack(pc);
            pc = ((opcode & 0x3FC) << 6) | code;
            break;
do_branch:
            pc = ((opcode & 0x3FC) << 6) | code;
no_branch: 
            gsub = 0;
            break;
        case 2:					// arith
            opArith((uint8_t)(code));
            gsub = 0;
            break;
        default: {				// goto
            uint16_t offset = (code >> 1) & 0x3F;
            if ((code & 0x80) != 0) offset -= 64;
            if ((code & 0x01) != 0) {
                if (p_cy != 0) pc += offset - 1;
            } else {
                if (p_cy == 0) pc += offset - 1;
            }
            gsub = 0;
            break;
        }
    } 
    p_cy = cy;
}
	
uint8_t nutLoop() {
    do {
        if (nutSleep) {
            if (wake) {
#ifdef DEBUG
                extapp_drawTextSmall("Wake ", 2, 36, 0xFFFF, 0x0000, false);
                extapp_drawTextSmall("                   ", 2, 60, 0xFFFF, 0x0000, false);
#endif
                wake = 0;
                nutSleep = 0;
                p_cy = w_cy;
                cy = p_cy;
                delayLeave = 1;         // stay on
            } else {
                delayLeave -= 65;
                if (delayLeave < 0) return 1;
                dcycles += 65;          // 0.01 sec
                extapp_msleep(9);       // so 9 ms :)
                phiLoop();
            }
        } else {
            evalOpcode();
            if (!nutSleep) evalOpcode();
            if (!nutSleep) evalOpcode();
            if (!nutSleep) evalOpcode();
        }
        scannerLoop();
    } while (dcycles < 130);            // 0.02 sec 
    dcycles -= 130;
    return 0;
}

uint8_t* nutSave(uint8_t* output) {
    uint8_t* _ptr=output;
    _ptr += copy(_ptr, (uint8_t*)(&config), sizeof(config));
    _ptr += copy(_ptr, rA, sizeof(rA));
    _ptr += copy(_ptr, rB, sizeof(rB));
    _ptr += copy(_ptr, rC, sizeof(rC));
    _ptr += copy(_ptr, rM, sizeof(rM));
    _ptr += copy(_ptr, rN, sizeof(rN));
    _ptr += copy(_ptr, rG, sizeof(rG));
    _ptr += copy(_ptr, fFi, sizeof(fFi));
    _ptr += copy(_ptr, fFo, sizeof(fFo));
    _ptr += copy(_ptr, fS, sizeof(fS));
    _ptr += copy(_ptr, (uint8_t*)(stack), sizeof(stack));
    _ptr += copy(_ptr, (uint8_t*)(&pc), sizeof(pc));
    _ptr += copy(_ptr, (uint8_t*)(&adr), sizeof(adr));
    _ptr += copy(_ptr, (uint8_t*)(&ptr), sizeof(ptr));
    _ptr += copy(_ptr, (uint8_t*)(&pp), sizeof(pp));
    _ptr += copy(_ptr, (uint8_t*)(&pq), sizeof(pq));
    _ptr += copy(_ptr, (uint8_t*)(&base), sizeof(base));
    _ptr += copy(_ptr, (uint8_t*)(&cy), sizeof(cy));
    _ptr += copy(_ptr, (uint8_t*)(&p_cy), sizeof(p_cy));
    _ptr += copy(_ptr, (uint8_t*)(&w_cy), sizeof(w_cy));
    _ptr += copy(_ptr, (uint8_t*)(&wake), sizeof(wake));
    int p;
    for (p = 0; p < 16; p++) {
        _ptr += copy(_ptr, &bank[p].curBank, sizeof(bank[p].curBank));
    }
    int i = 0; 
    int j = 0x01;
    for (p=0; p<1024; p++) {
        if (ramWr[i] & j) { _ptr += copy(_ptr, (uint8_t*)(&ram[p][0]), 7); }
        if (j == 0x80) { j = 0x01; i += 1; }
        else j += j;
    }
    return _ptr;
}

uint8_t* nutLoad(uint8_t* input) {
    uint8_t* _ptr=input;
    uint32_t configL;
    _ptr += copy((uint8_t*)(&configL), _ptr, sizeof(config));
    if (configL != config) return NULL; // force MEMORY LOST
    _ptr += copy(rA, _ptr, sizeof(rA));
    _ptr += copy(rB, _ptr, sizeof(rB));
    _ptr += copy(rC, _ptr, sizeof(rC));
    _ptr += copy(rM, _ptr, sizeof(rM));
    _ptr += copy(rN, _ptr, sizeof(rN));
    _ptr += copy(rG, _ptr, sizeof(rG));
    _ptr += copy(fFi, _ptr, sizeof(fFi));
    _ptr += copy(fFo, _ptr, sizeof(fFo));
    _ptr += copy(fS, _ptr, sizeof(fS));
    _ptr += copy((uint8_t*)(stack), _ptr, sizeof(stack));
    _ptr += copy((uint8_t*)(&pc), _ptr, sizeof(pc));
    _ptr += copy((uint8_t*)(&adr), _ptr, sizeof(adr));
    _ptr += copy((uint8_t*)(&ptr), _ptr, sizeof(ptr));
    _ptr += copy((uint8_t*)(&pp), _ptr, sizeof(pp));
    _ptr += copy((uint8_t*)(&pq), _ptr, sizeof(pq));
    _ptr += copy((uint8_t*)(&base), _ptr, sizeof(base));
    _ptr += copy((uint8_t*)(&cy), _ptr, sizeof(cy));
    _ptr += copy((uint8_t*)(&p_cy), _ptr, sizeof(p_cy));
    _ptr += copy((uint8_t*)(&w_cy), _ptr, sizeof(w_cy));
    _ptr += copy((uint8_t*)(&wake), _ptr, sizeof(wake));
    int p;
    for (p = 0; p < 16; p++) {
        _ptr += copy(&bank[p].curBank, _ptr, sizeof(bank[p].curBank));
    }
    int i = 0; 
    int j = 0x01;
    for (p=0; p<1024; p++) {
        if (ramWr[i] & j) { _ptr += copy((uint8_t*)(&ram[p][0]), _ptr, 7); }
        else regZero((uint8_t*)(&ram[p][0]), 7);
        if (j == 0x80) { j = 0x01; i += 1; }
        else j += j;
    }
    return _ptr;
}
