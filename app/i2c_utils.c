#include <msp430.h>
#include "i2c_utils.h"

#define MAX_I2C_LEN 64

char Screen_Data_Buffer[MAX_I2C_LEN];
int Screen_Data_Len = 0;
int Screen_Data_Cnt = 0;

char Matrix_Data_Buffer[MAX_I2C_LEN];
int Matrix_Data_Len = 0;
int Matrix_Data_Cnt = 0;

void i2c_write(int device, char data[], int len) {
    int i;
    __disable_interrupt();

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
    __enable_interrupt();
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

// ISR for screen (UCB0)
#pragma vector=EUSCI_B0_VECTOR
__interrupt void I2C_LED_SCREEN_ISR(void) {
    switch (__even_in_range(UCB0IV, USCI_I2C_UCBIT9IFG)) {
        case USCI_I2C_UCTXIFG0:
            if (Screen_Data_Cnt < Screen_Data_Len)
                UCB0TXBUF = Screen_Data_Buffer[Screen_Data_Cnt++];
            else
                UCB0IE &= ~UCTXIE0;
            break;
        case USCI_I2C_UCSTPIFG:
            UCB0IFG &= ~UCSTPIFG;
            break;
    }
}

// ISR for matrix (UCB1)
#pragma vector=EUSCI_B1_VECTOR
__interrupt void MATRIX_I2C_ISR(void) {
    switch (__even_in_range(UCB1IV, USCI_I2C_UCBIT9IFG)) {
        case USCI_I2C_UCTXIFG0:
            if (Matrix_Data_Cnt < Matrix_Data_Len)
                UCB1TXBUF = Matrix_Data_Buffer[Matrix_Data_Cnt++];
            else
                UCB1IE &= ~UCTXIE0;
            break;
        case USCI_I2C_UCSTPIFG:
            UCB1IFG &= ~UCSTPIFG;
            break;
    }
}
