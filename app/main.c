#include <msp430.h>

// Variable Declarations
int screen = 0;                             // device value for led screen
int matrix = 1;                             // device value for led matrix
int rotate = 0;                             // flag to indicate switch presses
int direction = 0;                          // tracks joystick movements
int center = 0;                             // direction value for no movement
int left = 1;                               // direction value for left movement
int right = 2;                              // direction value for right movement
int adc_value = 2000;                       // stores adc value that is read in
int lower_bound = 1000;                     // lower threshold for joystick adc
int upper_bound = 3000;                     // upper threshold for joystick adc

#define MAX_I2C_LEN 64
char Screen_Data_Buffer[MAX_I2C_LEN];
int Screen_Data_Len = 0;
int Screen_Data_Cnt = 0;
char Matrix_Data_Buffer[MAX_I2C_LEN];
int Matrix_Data_Len = 0;                    
int Matrix_Data_Cnt = 0;                   

char screen_init_cmds[] = {
    0x00,                                   // Control byte: command stream follows
    0xAE,                                   // Display OFF
    0xD5, 0x80,                             // Set display clock divide ratio/oscillator frequency
    0xA8, 0x3F,                             // Set multiplex ratio (height - 1); 0x3F = 64MUX
    0xD3, 0x00,                             // Set display offset to 0
    0x40,                                   // Set display start line to 0
    0x8D, 0x14,                             // Enable charge pump
    0x20, 0x00,                             // Memory addressing mode: Horizontal
    0xA1,                                   // Set segment re-map (column address 127 is mapped to SEG0)
    0xC8,                                   // Set COM output scan direction: remapped mode (COM[N-1] to COM0)
    0xDA, 0x12,                             // Set COM pins hardware configuration
    0x81, 0xCF,                             // Set contrast control
    0xD9, 0xF1,                             // Set pre-charge period
    0xDB, 0x40,                             // Set VCOMH deselect level
    0xA4,                                   // Resume to RAM content display
    0xA6,                                   // Normal display (not inverted)
    0xAF                                    // Display ON
};

char matrix_init_cmds[] = {
    0x21,                                   // Turn on oscillator
    0x81,                                   // Display on, no blink
    0xEF                                    // Full brightness
};

void init_system_timer(void) {
    TB0CTL |= (TBSSEL__ACLK | MC__UP | TBCLR);  // Use ACLK, up mode, clear
    TB0CCR0 = 32768;                        // 1.0s                                     
    TB0CCTL0 |= CCIE;                       // enable interrupts
    TB0CCTL0 &= ~CCIFG;                     // clear interrupt flag
    __enable_interrupt();                   // Enable interrupts
}

void init_joystick(void) {
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
    __enable_interrupt();                   // Enable interrupts
}

void init_push_button(void) {
    P1DIR &= ~BIT1;                         // Config P1.1 as input
    P1REN |= BIT1;                          // Enable resistor
    P1OUT |= BIT1;                          // Make pull up resistor
    P1IES |= BIT1;                          // Confug IRQ sensitivity H-to-L

    P1IFG &= ~BIT1;                         // clear interrupt flag
    P1IE |= BIT1;                           // Enable P1.1 IRQ
    __enable_interrupt();                   // Enable interrupts
}

void init_led_screen(void) {
    UCB0CTLW0 |= UCSWRST;                   // Put eUSCI_B0 in SW Reset
    UCB0CTLW0 |= UCSSEL__SMCLK;             // Choose BRCLK = SMCLK = 1Mhz
    UCB0BRW = 10;                           // Divide BRCLK by 10 for SCL = 100kHz
    UCB0CTLW0 |= UCMODE_3;                  // Put into I2C mode
    UCB0CTLW0 |= UCMST;                     // Put into master mode
    UCB0CTLW0 |= UCTR;                      // Put into Tx mode
    UCB0I2CSA = 0b01111000;                 // LED Screen address = 0b01111000
    UCB0CTLW1 |= UCASTP_2;                  // Auto STOP when UCB0TBCNT reached

    P1SEL1 &= ~BIT3;                        // P1.3 = SCL
    P1SEL0 |= BIT3;                            
    P1SEL1 &= ~BIT2;                        // P1.2 = SDA
    P1SEL0 |= BIT2;

    UCB0CTLW0 &= ~UCSWRST;                  // Take eUSCI_B0 out of SW Reset

    UCB0IE |= UCTXIE0;                      // Enable I2C Tx0 IR1
    __enable_interrupt();                   // Enable Maskable IRQs
}

void init_led_matrix(void) {
    UCB1CTLW0 |= UCSWRST;                   // Put eUSCI_B1 in SW Reset
    UCB1CTLW0 |= UCSSEL__SMCLK;             // Choose BRCLK = SMCLK = 1Mhz
    UCB1BRW = 10;                           // Divide BRCLK by 10 for SCL = 100kHz
    UCB1CTLW0 |= UCMODE_3;                  // Put into I2C mode
    UCB1CTLW0 |= UCMST;                     // Put into master mode
    UCB1CTLW0 |= UCTR;                      // Put into Tx mode
    UCB1I2CSA = 0x0070;                     // LED Matrix address = 0x70
    UCB1CTLW1 |= UCASTP_2;                  // Auto STOP when UCB0TBCNT reached

    P4SEL1 &= ~BIT7;                        // P4.7 = SCL
    P4SEL0 |= BIT7;                            
    P4SEL1 &= ~BIT6;                        // P4.7 = SDA
    P4SEL0 |= BIT6;

    UCB1CTLW0 &= ~UCSWRST;                  // Take eUSCI_B1 out of SW Reset

    UCB1IE |= UCTXIE0;                      // Enable I2C Tx0 IR1
    __enable_interrupt();                   // Enable Maskable IRQs
}

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;               // Stop watchdog timer           
    PM5CTL0 &= ~LOCKLPM5;                   // Disable High Z mode

    init_system_timer();
    init_joystick();
    init_push_button();
    init_led_screen();
    init_led_matrix();

    i2c_write(screen, screen_init_cmds, sizeof(screen_init_cmds));
    i2c_write(matrix, matrix_init_cmds, sizeof(matrix_init_cmds));

    while(1) {
        __no_operation();
    }
}   

void i2c_write(int device, char data[], int len) {
    int i;
    switch (device) {
        case 0: // screen
            for (i = 0; i < len; i++) {
                Screen_Data_Buffer[i] = data[i];
            }
            Screen_Data_Len = len;
            Screen_Data_Cnt = 0;
            UCB0TBCNT = len;
            UCB0CTLW0 |= UCTXSTT;
            break;

        case 1: // matrix
            for (i = 0; i < len; i++) {
                Matrix_Data_Buffer[i] = data[i];
            }
            Matrix_Data_Len = len;
            Matrix_Data_Cnt = 0;
            UCB1TBCNT = len;
            UCB1CTLW0 |= UCTXSTT;
            break;
    }
}

#pragma vector = TIMER0_B0_VECTOR
__interrupt void System_Timer_ISR(void) {
    ADCCTL0 |= ADCENC | ADCSC;
    TB0CCTL0 &= ~CCIFG;
}

#pragma vector=ADC_VECTOR
__interrupt void Joystick_ISR(void){
    adc_value = ADCMEM0;                    // read ADC value

    if (adc_value < lower_bound) {          // (1.25V / 5V) * 4095 = 1023
        direction = left;
    } else if (adc_value > upper_bound) {   // (3.75V / 5V) * 4095 = 3071
        direction = right;
    } else {                                // 1000 < adc_value < 3000
        direction = center;
    }
}

#pragma vector=PORT1_VECTOR
__interrupt void Push_Button_ISR(void) {
    rotate = 1;
    P1IFG &= ~BIT1;     
}

#pragma vector=EUSCI_B0_VECTOR
__interrupt void I2C_LED_SCREEN_ISR(void) {
    if (Screen_Data_Cnt < Screen_Data_Len) {
        UCB0TXBUF = Screen_Data_Buffer[Screen_Data_Cnt++];
    } 
}

#pragma vector=EUSCI_B1_VECTOR
__interrupt void MATRIX_I2C_ISR(void){
    if (Matrix_Data_Cnt < Matrix_Data_Len) {
        UCB1TXBUF = Matrix_Data_Buffer[Matrix_Data_Cnt++];
    }
}