#include <msp430.h>
#include "i2c_utils.h"
#include "led_screen.h"
#include "led_matrix.h"

// Variable Declarations
int screen = 0;                             // device value for led screen
int matrix = 1;                             // device value for led matrix
int rotate = 0;                             // flag to indicate switch presses
int direction = 0;                          // tracks joystick movements
int center = 0;                             // direction value for no movement
int left = 1;                               // direction value for left movement
int right = 2;                              // direction value for right movement
int adc_value = 3000;                       // stores adc value that is read in
int lower_bound = 1000;                     // lower threshold for joystick adc
int upper_bound = 4000;                     // upper threshold for joystick adc

void init_inputs(void) {
    // Initialize ADC for Joystick 
    P1SEL1 |= BIT0;                         // Configure P1.0 Pin for A0
    P1SEL0 |= BIT0; 

    ADCCTL0 &= ~ADCSHT;                     // Clear ADCSHT from def. of ADCSHT=01
    ADCCTL0 |= ADCSHT_2;                    // Conversion Cycles = 16 (ADCSHT=10)
    ADCCTL0 |= ADCON;                       // Turn ADC ON
    ADCCTL1 |= ADCSSEL_2;                   // ADC Clock Source = SMCLK
    ADCCTL1 |= ADCSHP;                      // Sample signal source = sampling timer
    ADCCTL2 &= ~ADCRES;                     // Clear ADCRES from def. of ADCRES=01
    ADCCTL2 |= ADCRES_2;                    // Resolution = 12-bit (ADCRES = 10)
    ADCMCTL0 |= ADCINCH_0;                  // ADC Input Channel = A0 (P1.0)
    ADCIE |= ADCIE0;                        // Enable ADC Conv Complete IRQ

    // Initialize Push button
    P1DIR &= ~BIT1;                         // Config P1.1 as input
    P1REN |= BIT1;                          // Enable resistor
    P1OUT |= BIT1;                          // Make pull up resistor
    P1IES |= BIT1;                          // Confug IRQ sensitivity H-to-L
    P1IFG &= ~BIT1;                         // clear interrupt flag
    P1IE |= BIT1;                           // Enable P1.1 IRQ

    __enable_interrupt();                   // Enable interrupts
}

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;
    PM5CTL0 &= ~LOCKLPM5;

    init_i2c_screen();
    init_i2c_matrix();
    init_led_screen();
    //init_led_matrix();
    init_inputs();

    display_start_game();

    while (1) {
    }
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

#pragma vector=ADC_VECTOR
__interrupt void Joystick_ISR(void){
    // need ADCCTL0 |= ADCENC | ADCSC; to start
    adc_value = ADCMEM0;                    // read ADC value

    if (adc_value < lower_bound) {          // (1.25V / 5V) * 4095 = 1023
        direction = right;
    } else if (adc_value > upper_bound) {   // (3.75V / 5V) * 4095 = 3071
        direction = left;
    } else {                                // 1000 < adc_value < 3000
        direction = center;
    }
}

#pragma vector=PORT1_VECTOR
__interrupt void Push_Button_ISR(void) {
    rotate = 1;
    P1IFG &= ~BIT1;     
}