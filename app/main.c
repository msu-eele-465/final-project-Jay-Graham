#include <msp430.h>

// Variable Declarations
int Screen_Data_Cnt;         
char Screen_Data[17];
int Matrix_Data_Cnt;    
char Matrix_Data[17];

void init_led_screen(void) {
    WDTCTL = WDTPW | WDTHOLD;               // Stop watchdog timer           
    PM5CTL0 &= ~LOCKLPM5;                   // Disable High Z mode

    UCB0CTLW0 |= UCSWRST;                   // Put eUSCI_B0 in SW Reset
    UCB0CTLW0 |= UCSSEL__SMCLK;             // Choose BRCLK = SMCLK = 1Mhz
    UCB0BRW = 10;                           // Divide BRCLK by 10 for SCL = 100kHz
    UCB0CTLW0 |= UCMODE_3;                  // Put into I2C mode
    UCB0CTLW0 |= UCMST;                     // Put into master mode
    UCB0CTLW0 |= UCTR;                      // Put into Tx mode
    UCB0I2CSA = 0x0070;                     // LED Matrix address = 0x70
    UCB0CTLW1 |= UCASTP_2;                  // Auto STOP when UCB0TBCNT reached
    UCB0TBCNT = sizeof(Matrix_Data);        // # of bytes in packet

    P1SEL1 &= ~BIT3;                        // P1.3 = SCL
    P1SEL0 |= BIT3;                            
    P1SEL1 &= ~BIT2;                        // P1.2 = SDA
    P1SEL0 |= BIT2;

    UCB0CTLW0 &= ~UCSWRST;                  // Take eUSCI_B0 out of SW Reset

    UCB0IE |= UCTXIE0;                      // Enable I2C Tx0 IR1
    __enable_interrupt();                   // Enable Maskable IRQs
}

void init_led_matrix(void) {
    WDTCTL = WDTPW | WDTHOLD;               // Stop watchdog timer           
    PM5CTL0 &= ~LOCKLPM5;                   // Disable High Z mode

    UCB1CTLW0 |= UCSWRST;                   // Put eUSCI_B0 in SW Reset
    UCB1CTLW0 |= UCSSEL__SMCLK;             // Choose BRCLK = SMCLK = 1Mhz
    UCB1BRW = 10;                           // Divide BRCLK by 10 for SCL = 100kHz
    UCB1CTLW0 |= UCMODE_3;                  // Put into I2C mode
    UCB1CTLW0 |= UCMST;                     // Put into master mode
    UCB1CTLW0 |= UCTR;                      // Put into Tx mode
    UCB1I2CSA = 0x0070;                     // LED Matrix address = 0x70
    UCB1CTLW1 |= UCASTP_2;                  // Auto STOP when UCB0TBCNT reached
    UCB1TBCNT = sizeof(Matrix_Data);        // # of bytes in packet

    P4SEL1 &= ~BIT7;                        // P4.7 = SCL
    P4SEL0 |= BIT7;                            
    P4SEL1 &= ~BIT6;                        // P4.7 = SDA
    P4SEL0 |= BIT6;

    UCB1CTLW0 &= ~UCSWRST;                  // Take eUSCI_B0 out of SW Reset

    UCB1IE |= UCTXIE0;                      // Enable I2C Tx0 IR1
    __enable_interrupt();                   // Enable Maskable IRQs
}

int main(void) {
    init_led_screen();
    init_led_matrix();

    while(1) {
        __no_operation();
    }
}   