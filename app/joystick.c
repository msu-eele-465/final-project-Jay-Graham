#include <msp430.h>
#include "joystick.h"

static volatile int rotate_flag = 0;        // Flag for pushbutton press
static const int lower_bound = 1000;        // ADC threshold for RIGHT
static const int upper_bound = 4000;        // ADC threshold for LEFT

void init_joystick(void) {
    // Configure joystick X-axis (ADC A0 on P1.0)
    P1SEL1 |= BIT0;
    P1SEL0 |= BIT0;

    ADCCTL0 &= ~ADCSHT;
    ADCCTL0 |= ADCSHT_2;                    // 16 ADC clock cycles
    ADCCTL0 |= ADCON;                       // Turn on ADC

    ADCCTL1 |= ADCSSEL_2;                   // SMCLK
    ADCCTL1 |= ADCSHP;                      // Sampling timer
    ADCCTL2 &= ~ADCRES;
    ADCCTL2 |= ADCRES_2;                    // 12-bit resolution

    ADCMCTL0 |= ADCINCH_0;                  // Use A0
    ADCIE &= ~ADCIE0;                       // We use polling, not interrupts

    // Configure pushbutton on P1.1
    P1DIR &= ~BIT1;                         // Input
    P1REN |= BIT1;                          // Enable pull-up/down
    P1OUT |= BIT1;                          // Pull-up
    P1IES |= BIT1;                          // High-to-low edge
    P1IFG &= ~BIT1;                         // Clear flag
    P1IE  |= BIT1;                          // Enable interrupt
}

int get_direction(void) {
    ADCCTL0 |= ADCENC | ADCSC;              // Start conversion
    while (!(ADCIFG & ADCIFG0));            // Wait for completion
    int value = ADCMEM0;

    if (value < lower_bound) return RIGHT;
    else if (value > upper_bound) return LEFT;
    else return CENTER;
}

int check_rotate(void) {
    int temp = rotate_flag;
    rotate_flag = 0;
    return temp;
}

// Push button ISR
#pragma vector=PORT1_VECTOR
__interrupt void PORT1_ISR(void) {
    if (P1IFG & BIT1) {
        rotate_flag = 1;
        P1IFG &= ~BIT1;
    }
}
