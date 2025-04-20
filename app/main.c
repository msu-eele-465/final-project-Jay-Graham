#include <msp430.h>
#include <oled_display.h>

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

#define MAX_I2C_LEN 64
char Screen_Data_Buffer[MAX_I2C_LEN];
int Screen_Data_Len = 0;
int Screen_Data_Cnt = 0;
char Matrix_Data_Buffer[MAX_I2C_LEN];
int Matrix_Data_Len = 0;                    
int Matrix_Data_Cnt = 0;    

#define OLED_WIDTH 128
#define OLED_HEIGHT 64

char screen_init_cmds[] = {
    0x00, 0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00,
    0x40, 0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8, 0xDA,
    0x12, 0x81, 0xCF, 0xD9, 0xF1, 0xDB, 0x40, 0xA4,
    0xA6, 0xAF
};

char matrix_init_cmds[] = {
    0x21, 0x81, 0xEF
};

char font6x8[][6] = {
    [' '] = { 0x00,0x00,0x00,0x00,0x00,0x00 }, 
    ['A'] = { 0x7C,0x12,0x11,0x12,0x7C,0x00 },
    ['B'] = { 0x7F,0x49,0x49,0x49,0x36,0x00 },
    ['C'] = { 0x3E,0x41,0x41,0x41,0x22,0x00 },
    ['D'] = { 0x7F,0x41,0x41,0x22,0x1C,0x00 },
    ['E'] = { 0x7F,0x49,0x49,0x49,0x41,0x00 },
    ['F'] = { 0x7F,0x09,0x09,0x09,0x01,0x00 },
    ['G'] = { 0x3E,0x41,0x49,0x49,0x7A,0x00 },
    ['H'] = { 0x7F,0x08,0x08,0x08,0x7F,0x00 },
    ['I'] = { 0x00,0x41,0x7F,0x41,0x00,0x00 },
    ['J'] = { 0x20,0x40,0x41,0x3F,0x01,0x00 },
    ['K'] = { 0x7F,0x08,0x14,0x22,0x41,0x00 },
    ['L'] = { 0x7F,0x40,0x40,0x40,0x40,0x00 },
    ['M'] = { 0x7F,0x02,0x0C,0x02,0x7F,0x00 },
    ['N'] = { 0x7F,0x04,0x08,0x10,0x7F,0x00 },
    ['O'] = { 0x3E,0x41,0x41,0x41,0x3E,0x00 },
    ['P'] = { 0x7F,0x09,0x09,0x09,0x06,0x00 },
    ['Q'] = { 0x3E,0x41,0x51,0x21,0x5E,0x00 },
    ['R'] = { 0x7F,0x09,0x19,0x29,0x46,0x00 },
    ['S'] = { 0x46,0x49,0x49,0x49,0x31,0x00 },
    ['T'] = { 0x01,0x01,0x7F,0x01,0x01,0x00 },
    ['U'] = { 0x3F,0x40,0x40,0x40,0x3F,0x00 },
    ['V'] = { 0x1F,0x20,0x40,0x20,0x1F,0x00 },
    ['W'] = { 0x7F,0x20,0x18,0x20,0x7F,0x00 },
    ['X'] = { 0x63,0x14,0x08,0x14,0x63,0x00 },
    ['Y'] = { 0x07,0x08,0x70,0x08,0x07,0x00 },
    ['Z'] = { 0x61,0x51,0x49,0x45,0x43,0x00 }
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
    UCB0I2CSA = 0x3C;                       // LED Screen address = 0b01111000
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

    clear_screen();
    display_start_screen();
    //display_game_over();

    while(1) {
        __no_operation();
    }
}   

void i2c_write(int device, char data[], int len) {
    int i;

    switch (device) {
        case 0: // screen
            while (UCB0STATW & UCBBUSY);
            while (UCB0CTLW0 & UCTXSTP); // wait for STOP

            for (i = 0; i < len; i++) Screen_Data_Buffer[i] = data[i];
            Screen_Data_Len = len;
            Screen_Data_Cnt = 0;

            UCB0TBCNT = len;
            UCB0CTLW0 |= UCTXSTT;
            break;

        case 1: // matrix
            while (UCB1STATW & UCBBUSY);
            while (UCB1CTLW0 & UCTXSTP); // wait for STOP

            for (i = 0; i < len; i++) Matrix_Data_Buffer[i] = data[i];
            Matrix_Data_Len = len;
            Matrix_Data_Cnt = 0;

            UCB1TBCNT = len;
            UCB1CTLW0 |= UCTXSTT;
            break;
    }
}

void clear_screen() {
    char set_range[] = {
        0x00,           // Command stream
        0x21, 0x00, 0x7F,
        0x22, 0x00, 0x07
    };
    i2c_write(screen, set_range, sizeof(set_range));

    // One page = 128 data bytes, so split into two 64-byte transfers
    char buffer1[65], buffer2[65];
    buffer1[0] = 0x40;  // Data control byte
    buffer2[0] = 0x40;
    int i;
    for (i = 1; i < 65; i++) {
        buffer1[i] = 0x00;
        buffer2[i] = 0x00;
    }
    int page;
    for (page = 0; page < 8; page++) {
        i2c_write(screen, buffer1, sizeof(buffer1));
        i2c_write(screen, buffer2, sizeof(buffer2));
    }
}

void draw_string(int col, int page, const char *str) {
    char buffer[1 + 6 * 21];  // Up to 21 characters max for safety
    int i = 0;
    buffer[0] = 0x40;         // Control byte for data stream

    while (*str && i < 21) {
        int j;
        for (j = 0; j < 6; j++) {
            buffer[1 + i * 6 + j] = font6x8[(int)*str][j];
        }
        str++;
        i++;
    }

    // Set page and column
    char set_cursor[] = {
        0x00,
        0x21, col, 127,
        0x22, page, page
    };
    i2c_write(screen, set_cursor, sizeof(set_cursor));
    i2c_write(screen, buffer, 1 + i * 6);
}

void display_start_screen() {
    clear_screen();
    draw_string(0, 1, "     MINI TETRIS     ");
    draw_string(0, 4, "  CLICK JOYSTICK TO  ");
    draw_string(0, 5, "        START        ");
}

void display_game_over() {
    clear_screen();
    draw_string(0, 1, "     MINI TETRIS     ");
    draw_string(0, 4, "      GAME OVER      ");
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

#pragma vector=EUSCI_B1_VECTOR
__interrupt void MATRIX_I2C_ISR(void){
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