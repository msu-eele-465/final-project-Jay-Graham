#include <msp430.h>

// I2C BUFFERS
#define MAX_I2C_LEN 64

char Screen_Data_Buffer[MAX_I2C_LEN];
int Screen_Data_Len = 0;
int Screen_Data_Cnt = 0;

char Matrix_Data_Buffer[MAX_I2C_LEN];
int Matrix_Data_Len = 0;
int Matrix_Data_Cnt = 0;

// I2C ADDRESSES
#define SCREEN_I2C_ADDR 0x3C
#define MATRIX_I2C_ADDR 0x70
#define LED_MATRIX_DEVICE_ID 1

// FONT TABLE
static const unsigned char font6x8[][6] = {
    [' '] = { 0x00,0x00,0x00,0x00,0x00,0x00 },
    ['A'] = { 0x7C,0x12,0x11,0x12,0x7C,0x00 },
    ['C'] = { 0x3E,0x41,0x41,0x41,0x22,0x00 },
    ['E'] = { 0x7F,0x49,0x49,0x49,0x41,0x00 },
    ['G'] = { 0x3E,0x41,0x49,0x49,0x7A,0x00 },
    ['I'] = { 0x00,0x41,0x7F,0x41,0x00,0x00 },
    ['J'] = { 0x20,0x40,0x41,0x3F,0x01,0x00 },
    ['K'] = { 0x7F,0x08,0x14,0x22,0x41,0x00 },
    ['L'] = { 0x7F,0x40,0x40,0x40,0x40,0x00 },
    ['M'] = { 0x7F,0x02,0x0C,0x02,0x7F,0x00 },
    ['N'] = { 0x7F,0x04,0x08,0x10,0x7F,0x00 },
    ['O'] = { 0x3E,0x41,0x41,0x41,0x3E,0x00 },
    ['R'] = { 0x7F,0x09,0x19,0x29,0x46,0x00 },
    ['S'] = { 0x46,0x49,0x49,0x49,0x31,0x00 },
    ['T'] = { 0x01,0x01,0x7F,0x01,0x01,0x00 },
    ['V'] = { 0x1F,0x20,0x40,0x20,0x1F,0x00 },
    ['0'] = { 0x3E, 0x45, 0x49, 0x51, 0x3E, 0x00 },
    ['1'] = { 0x00, 0x42, 0x7F, 0x40, 0x00, 0x00 },
    ['2'] = { 0x62, 0x51, 0x49, 0x49, 0x46, 0x00 },
    ['3'] = { 0x22, 0x49, 0x49, 0x49, 0x36, 0x00 },
    ['4'] = { 0x18, 0x14, 0x12, 0x7F, 0x10, 0x00 },
    ['5'] = { 0x2F, 0x49, 0x49, 0x49, 0x31, 0x00 },
    ['6'] = { 0x3E, 0x49, 0x49, 0x49, 0x32, 0x00 },
    ['7'] = { 0x01, 0x71, 0x09, 0x05, 0x03, 0x00 },
    ['8'] = { 0x36, 0x49, 0x49, 0x49, 0x36, 0x00 },
    ['9'] = { 0x26, 0x49, 0x49, 0x49, 0x3E, 0x00 }
};

// GLOBAL FLAGS
int rotate_flag = 0;
int left_flag = 0;
int right_flag = 0;
int adc_value = 3000;
int lower_bound = 1000;
int upper_bound = 4000;

// INIT FUNCTIONS
void init_i2c_screen(void) {
    UCB0CTLW0 |= UCSWRST;
    UCB0CTLW0 |= UCSSEL__SMCLK | UCMODE_3 | UCMST | UCTR;
    UCB0BRW = 10;
    UCB0I2CSA = SCREEN_I2C_ADDR;
    UCB0CTLW1 |= UCASTP_2;
    P1SEL1 &= ~BIT3;
    P1SEL0 |=  BIT3;
    P1SEL1 &= ~BIT2;
    P1SEL0 |=  BIT2;
    UCB0CTLW0 &= ~UCSWRST;
    UCB0IE |= UCTXIE0;
    __enable_interrupt();
}

void init_i2c_matrix(void) {
    UCB1CTLW0 |= UCSWRST;
    UCB1CTLW0 |= UCSSEL__SMCLK | UCMODE_3 | UCMST | UCTR;
    UCB1BRW = 10;
    UCB1I2CSA = MATRIX_I2C_ADDR;
    UCB1CTLW1 |= UCASTP_2;
    P4SEL1 &= ~BIT7;
    P4SEL0 |=  BIT7;
    P4SEL1 &= ~BIT6;
    P4SEL0 |=  BIT6;
    UCB1CTLW0 &= ~UCSWRST;
    UCB1IE |= UCTXIE0;
    __enable_interrupt();
}

void init_inputs(void) {
    P1SEL1 |= BIT0;
    P1SEL0 |= BIT0;

    ADCCTL0 &= ~ADCSHT;
    ADCCTL0 |= ADCSHT_2;
    ADCCTL0 |= ADCON;
    ADCCTL1 |= ADCSSEL_2;
    ADCCTL1 |= ADCSHP;
    ADCCTL2 &= ~ADCRES;
    ADCCTL2 |= ADCRES_2;
    ADCMCTL0 |= ADCINCH_0;
    ADCIE |= ADCIE0;

    P1DIR &= ~BIT1;
    P1REN |= BIT1;
    P1OUT |= BIT1;
    P1IES |= BIT1;
    P1IFG &= ~BIT1;
    P1IE |= BIT1;

    __enable_interrupt();
}

// MAIN FUNCTION
int main(void) {
    WDTCTL = WDTPW | WDTHOLD;
    PM5CTL0 &= ~LOCKLPM5;

    init_inputs();
    init_i2c_screen();
    init_i2c_matrix();
    init_led_screen();
    init_led_matrix();

    display_start_game();

    while (1);
}

// I2C WRITE
void i2c_write(int device, char *data, int len) {
    int i;
    __disable_interrupt();

    if (device == 0) {
        while (UCB0STATW & UCBBUSY);
        while (UCB0CTLW0 & UCTXSTP);
        for (i = 0; i < len; i++) Screen_Data_Buffer[i] = data[i];
        Screen_Data_Len = len;
        Screen_Data_Cnt = 0;
        UCB0TBCNT = len;
        UCB0IE |= UCTXIE0;
        UCB0CTLW0 |= UCTXSTT;
    } else {
        while (UCB1STATW & UCBBUSY);
        while (UCB1CTLW0 & UCTXSTP);
        for (i = 0; i < len; i++) Matrix_Data_Buffer[i] = data[i];
        Matrix_Data_Len = len;
        Matrix_Data_Cnt = 0;
        UCB1TBCNT = len;
        UCB1IE |= UCTXIE0;
        UCB1CTLW0 |= UCTXSTT;
    }

    __enable_interrupt();
}

// LED INIT
void init_led_screen(void) {
    static const char oled_init_cmds[] = {
        0x00, 0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00,
        0x40, 0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8, 0xDA,
        0x12, 0x81, 0xCF, 0xD9, 0xF1, 0xDB, 0x40, 0xA4,
        0xA6, 0xAF
    };
    i2c_write(0, (char *)oled_init_cmds, sizeof(oled_init_cmds));
}

void init_led_matrix(void) {
    static const char matrix_init_cmds[][2] = {
        { 0x00, 0x21 },  
        { 0x00, 0x81 },  
        { 0x00, 0xEF }   
    };
    int i;
    for (i = 0; i < 3; i++) {
        i2c_write(LED_MATRIX_DEVICE_ID, (char *)matrix_init_cmds[i], 2);
    }
}

// OLED FUNCTIONS
void clear_screen(void) {
    char set_range[] = {0x00, 0x21, 0x00, 0x7F, 0x22, 0x00, 0x07};
    i2c_write(0, set_range, sizeof(set_range));

    char buffer[65];
    buffer[0] = 0x40;
    int i;
    for (i = 1; i < 65; i++) buffer[i] = 0x00;
    int page;
    for (page = 0; page < 8; page++) {
        i2c_write(0, buffer, sizeof(buffer));
        i2c_write(0, buffer, sizeof(buffer));
    }
}

void draw_string(int col, int page, const char *str) {
    char buffer[1 + 6 * 21];
    buffer[0] = 0x40;

    int i = 0;
    int j = 0;
    while (*str && i < 21) {
        for (j = 0; j < 6; j++) {
            buffer[1 + i * 6 + j] = font6x8[(int)*str][j];
        }
        str++;
        i++;
    }

    char set_cursor[] = { 0x00, 0x21, col, 127, 0x22, page, page };
    i2c_write(0, set_cursor, sizeof(set_cursor));
    i2c_write(0, buffer, 1 + i * 6);
}

void display_start_game(void) {
    clear_screen();
    draw_string(0, 1, "     MINI TETRIS     ");
    draw_string(0, 4, "  CLICK JOYSTICK TO  ");
    draw_string(0, 5, "        START        ");
}

void display_game_over(void) {
    clear_screen();
    draw_string(0, 1, "     MINI TETRIS     ");
    draw_string(0, 4, "      GAME OVER      ");
}

void display_score(int score) {
    clear_screen();
    draw_string(0, 1, "     MINI TETRIS     ");
    draw_string(0, 4, "        SCORE        ");

    char buffer[22];
    int j = 0;
    for (j = 0; j < 21; j++) buffer[j] = ' ';
    buffer[21] = '\0';

    if (score >= 100) {
        buffer[9] = (score / 100) + '0';
        buffer[10] = ((score / 10) % 10) + '0';
        buffer[11] = (score % 10) + '0';
    } else if (score >= 10) {
        buffer[10] = ((score / 10) % 10) + '0';
        buffer[11] = (score % 10) + '0';
    } else {
        buffer[11] = (score % 10) + '0';
    }

    draw_string(0, 5, buffer);
}

// INTERRUPTS
#pragma vector=ADC_VECTOR
__interrupt void Joystick_ISR(void) {
    adc_value = ADCMEM0;

    if (adc_value < lower_bound) {
        right_flag = 1;
        left_flag = 0;
    } else if (adc_value > upper_bound) {
        left_flag = 1;
        right_flag = 0;
    } else {
        right_flag = 0;
        left_flag = 0;
    }
}

#pragma vector=PORT1_VECTOR
__interrupt void Push_Button_ISR(void) {
    rotate_flag = 1;
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
