#include <msp430.h>
#include "i2c_utils.h"

char Screen_Data_Buffer[];
int Screen_Data_Len;
int Screen_Data_Cnt;

char Matrix_Data_Buffer[];
int Matrix_Data_Len;
int Matrix_Data_Cnt;

void i2c_write(int device, char data[], int len) {
    int i;

    switch (device) {
        case 0: // screen
            while (UCB0STATW & UCBBUSY);
            while (UCB0CTLW0 & UCTXSTP);
            for (i = 0; i < len; i++) Screen_Data_Buffer[i] = data[i];
            Screen_Data_Len = len;
            Screen_Data_Cnt = 0;
            UCB0TBCNT = len;
            UCB0CTLW0 |= UCTXSTT;
            break;

        case 1: // matrix
            while (UCB1STATW & UCBBUSY);
            while (UCB1CTLW0 & UCTXSTP);
            for (i = 0; i < len; i++) Matrix_Data_Buffer[i] = data[i];
            Matrix_Data_Len = len;
            Matrix_Data_Cnt = 0;
            UCB1TBCNT = len;
            UCB1CTLW0 |= UCTXSTT;
            break;
    }
}

void init_i2c_screen(void) {
    UCB0CTLW0 |= UCSWRST;
    UCB0CTLW0 |= UCSSEL__SMCLK | UCMODE_3 | UCMST | UCTR;
    UCB0BRW = 10;               // 100kHz
    UCB0I2CSA = 0x3C;           // Screen address
    UCB0CTLW1 |= UCASTP_2;      // Auto STOP when TBCNT hits
    P1SEL1 &= ~BIT3;
    P1SEL0 |=  BIT3;            // P1.3 = SCL
    P1SEL1 &= ~BIT2;
    P1SEL0 |=  BIT2;            // P1.2 = SDA
    UCB0CTLW0 &= ~UCSWRST;
    UCB0IE |= UCTXIE0;
    __enable_interrupt();
}

void init_i2c_matrix(void) {
    UCB1CTLW0 |= UCSWRST;
    UCB1CTLW0 |= UCSSEL__SMCLK | UCMODE_3 | UCMST | UCTR;
    UCB1BRW = 10;               // 100kHz
    UCB1I2CSA = 0x70;           // Matrix address
    UCB1CTLW1 |= UCASTP_2;      // Auto STOP when TBCNT hits
    P4SEL1 &= ~BIT7;
    P4SEL0 |=  BIT7;            // P4.7 = SCL
    P4SEL1 &= ~BIT6;
    P4SEL0 |=  BIT6;            // P4.6 = SDA
    UCB1CTLW0 &= ~UCSWRST;
    UCB1IE |= UCTXIE0;
    __enable_interrupt();
}
