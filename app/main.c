#include <msp430.h>

// I2C BUFFERS
#define MAX_I2C_LEN 64
char Screen_Data_Buffer[MAX_I2C_LEN];
int Screen_Data_Len = 0;
int Screen_Data_Cnt = 0;

// I2C ADDRESSES
#define SCREEN_I2C_ADDR 0x3C

// ASCII-indexed 6x8 font (only needed characters)
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

// I2C INIT
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

// MAIN
int main(void) {
    WDTCTL = WDTPW | WDTHOLD;
    PM5CTL0 &= ~LOCKLPM5;

    init_i2c_screen();
    init_led_screen();

    __delay_cycles(1000000);
    display_start_game();

    while (1);
}

// BASIC I2C WRITE
void i2c_write(char *data, int len) {
    int i;
    __disable_interrupt();
    while (UCB0STATW & UCBBUSY);
    while (UCB0CTLW0 & UCTXSTP);

    for (i = 0; i < len && i < MAX_I2C_LEN; i++) {
        Screen_Data_Buffer[i] = data[i];
    }

    Screen_Data_Len = len;
    Screen_Data_Cnt = 0;
    UCB0TBCNT = len;
    UCB0IE |= UCTXIE0;
    UCB0CTLW0 |= UCTXSTT;
    __enable_interrupt();
}

// OLED INIT
void init_led_screen(void) {
    static const char oled_init_cmds[] = {
        0x00, 0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00,
        0x40, 0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8, 0xDA,
        0x12, 0x81, 0xCF, 0xD9, 0xF1, 0xDB, 0x40, 0xA4,
        0xA6, 0xAF
    };
    i2c_write((char *)oled_init_cmds, sizeof(oled_init_cmds));
}

// CLEAR SCREEN
void clear_screen(void) {
    const char set_range[] = {
        0x00, 0x21, 0x00, 0x7F,
               0x22, 0x00, 0x07
    };
    i2c_write((char *)set_range, sizeof(set_range));

    char buffer[33];
    buffer[0] = 0x40;
    int i;
    for (i = 1; i < 33; i++) buffer[i] = 0x00;

    for (i = 0; i < 32; i++) {
        while (UCB0STATW & UCBBUSY);
        i2c_write(buffer, sizeof(buffer));
    }
}

// DRAW STRING
void draw_string(char col, char page, const char *text) {
    char buffer[7];
    buffer[0] = 0x40;

    while (*text) {
        const unsigned char *glyph = font6x8[(int)*text];
        int i;
        for (i = 0; i < 6; i++) buffer[i + 1] = glyph ? glyph[i] : 0x00;

        char cmd[] = { 0x00, 0x21, col, col + 5, 0x22, page, page };
        while (UCB0STATW & UCBBUSY);
        i2c_write(cmd, sizeof(cmd));
        while (UCB0STATW & UCBBUSY);
        i2c_write(buffer, 7);

        col += 6;
        text++;
    }
}

// DISPLAY START GAME
void display_start_game(void) {
    clear_screen();
    draw_string(0, 1, "     MINI TETRIS     ");
    draw_string(0, 4, "  CLICK JOYSTICK TO  ");
    draw_string(0, 5, "        START        ");
}

// DISPLAY GAME OVER
void display_game_over(void) {
    clear_screen();
    draw_string(0, 1, "     MINI TETRIS     ");
    draw_string(0, 4, "      GAME OVER      ");
}

// DISPLAY SCORE
void display_score(int score) {
    clear_screen();
    draw_string(0, 1, "     MINI TETRIS     ");
    draw_string(0, 4, "        SCORE        ");

    char buffer[22];
    int j;
    for (j = 0; j < 21; j++) buffer[j] = ' ';
    buffer[21] = '\0';

    if (score >= 100) {
        buffer[9]  = (score / 100) + '0';
        buffer[10] = (score / 10) % 10 + '0';
        buffer[11] = (score % 10) + '0';
    } else if (score >= 10) {
        buffer[10] = (score / 10) % 10 + '0';
        buffer[11] = (score % 10) + '0';
    } else {
        buffer[11] = (score % 10) + '0';
    }

    draw_string(0, 5, buffer);
}

// I2C ISR
#pragma vector=EUSCI_B0_VECTOR
__interrupt void I2C_ISR(void) {
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
