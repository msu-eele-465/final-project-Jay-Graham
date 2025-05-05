#include <msp430.h>

// I2C BUFFERS
#define MAX_I2C_LEN 64

char Screen_Data_Buffer[MAX_I2C_LEN];
int Screen_Data_Len = 0;
int Screen_Data_Cnt = 0;

char Matrix_Data_Buffer[MAX_I2C_LEN];
int Matrix_Data_Len = 0;
int Matrix_Data_Cnt = 0;

// I2C ADDRESSES AND DEVICE IDS
#define SCREEN_I2C_ADDR 0x3C
#define MATRIX_I2C_ADDR 0x70
#define LED_SCREEN_DEVICE_ID 0
#define LED_MATRIX_DEVICE_ID 1

// TETRIS GAME VARIABLES
char game_field[16][8];     
int active_row = -1;       
int active_col = 3;         
int block_active = 0; 
int rows_cleared = 0;
int last_score = -1;
int last_level = -1;
int displayed_start = 0;
int displayed_game_over = 0;

// TETRIS GAME START
#define STATE_START     0
#define STATE_PLAYING   1
#define STATE_GAME_OVER 2
int game_state = STATE_START;

// JOYSTICK FLAGS
int pb_flag = 0;
int left_flag = 0;
int right_flag = 0;
int adc_value = 2000;
int lower_bound = 1000;
int upper_bound = 3000;
int level = 0;
int score = 0;

// ASCII-indexed 6x8 font
static const unsigned char font6x8[][6] = {
    [' '] = { 0x00,0x00,0x00,0x00,0x00,0x00 },
    [':'] = { 0x00,0x36,0x36,0x00,0x00,0x00 },
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
    ['Y'] = { 0x07,0x08,0x70,0x08,0x07,0x00 },
    ['0'] = { 0x3E,0x45,0x49,0x51,0x3E,0x00 },
    ['1'] = { 0x00,0x42,0x7F,0x40,0x00,0x00 },
    ['2'] = { 0x62,0x51,0x49,0x49,0x46,0x00 },
    ['3'] = { 0x22,0x49,0x49,0x49,0x36,0x00 },
    ['4'] = { 0x18,0x14,0x12,0x7F,0x10,0x00 },
    ['5'] = { 0x2F,0x49,0x49,0x49,0x31,0x00 },
    ['6'] = { 0x3E,0x49,0x49,0x49,0x32,0x00 },
    ['7'] = { 0x01,0x71,0x09,0x05,0x03,0x00 },
    ['8'] = { 0x36,0x49,0x49,0x49,0x36,0x00 },
    ['9'] = { 0x26,0x49,0x49,0x49,0x3E,0x00 }
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

void init_game_timer(void) {
    TB0CTL |= (TBSSEL__ACLK | MC__UP | TBCLR);  
    TB0CCR0 = 10000;                            
    TB0CCTL0 |= CCIE;                          
    TB0CCTL0 &= ~CCIFG;
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

// MAIN
int main(void) {
    WDTCTL = WDTPW | WDTHOLD;
    PM5CTL0 &= ~LOCKLPM5;

    init_i2c_screen();
    init_i2c_matrix();
    init_game_timer();
    init_inputs();
    init_led_screen();
    init_led_matrix();

    display_start_game();
    clear_matrix();
    clear_game_field();

    while (1) {
        ADCCTL0 |= ADCENC | ADCSC;

        switch (game_state) {
            case STATE_START:
                if (!displayed_start) {
                    display_start_game();
                    displayed_start = 1;
                }
                if (pb_flag) {
                    pb_flag = 0;
                    score = 0;
                    level = 1;
                    rows_cleared = 0;
                    last_score = -1;
                    last_level = -1;
                    clear_game_field();
                    clear_matrix();
                    game_state = STATE_PLAYING;
                    TB0CCR0 = 12000;  // Level 1 speed
                }
                break;
            case STATE_PLAYING:
                if (score != last_score || level != last_level) {
                    display_score();
                    last_score = score;
                    last_level = level;
                }
                break;
            case STATE_GAME_OVER:
                if (!displayed_game_over) {
                    display_game_over();
                    clear_game_field();
                    displayed_game_over = 1;
                    __delay_cycles(3000000);
                    displayed_start = 0;
                    displayed_game_over = 0;
                    pb_flag = 0;      
                    left_flag = 0;
                    right_flag = 0;
                    game_state = STATE_START;
                }
                break;
        }
    }
}

// I2C WRITE
void i2c_write(int device_id, char *data, int len) {
    int i;
    __disable_interrupt();

    if (device_id == 0) {
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
    } else if (device_id == 1) {
        while (UCB1STATW & UCBBUSY);
        while (UCB1CTLW0 & UCTXSTP);
        for (i = 0; i < len && i < MAX_I2C_LEN; i++) {
            Matrix_Data_Buffer[i] = data[i];
        }
        Matrix_Data_Len = len;
        Matrix_Data_Cnt = 0;
        UCB1TBCNT = len;
        UCB1IE |= UCTXIE0;
        UCB1CTLW0 |= UCTXSTT;
    }
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
    i2c_write(LED_SCREEN_DEVICE_ID, (char *)oled_init_cmds, sizeof(oled_init_cmds));
}

// CLEAR SCREEN
void clear_screen(void) {
    const char set_range[] = {
        0x00, 0x21, 0x00, 0x7F,
               0x22, 0x00, 0x07
    };
    i2c_write(LED_SCREEN_DEVICE_ID, (char *)set_range, sizeof(set_range));

    char buffer[33];
    buffer[0] = 0x40;
    int i;
    for (i = 1; i < 33; i++) buffer[i] = 0x00;

    for (i = 0; i < 32; i++) {
        while (UCB0STATW & UCBBUSY);
        i2c_write(LED_SCREEN_DEVICE_ID, buffer, sizeof(buffer));
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
        i2c_write(LED_SCREEN_DEVICE_ID, cmd, sizeof(cmd));
        while (UCB0STATW & UCBBUSY);
        i2c_write(LED_SCREEN_DEVICE_ID, buffer, 7);

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

void int_to_str(int val, char* buf) {
    // Handle 0 explicitly
    if (val == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }

    char temp[10];  // Holds digits in reverse
    int i = 0;

    while (val > 0 && i < 10) {
        temp[i++] = '0' + (val % 10);
        val /= 10;
    }

    // Reverse into buf
    int j = 0;
    while (i > 0) {
        buf[j++] = temp[--i];
    }
    buf[j] = '\0';
}

// DISPLAY SCORE
void display_score(void) {
    draw_string(0, 1, "     MINI TETRIS     ");

    char level_num[10];
    char score_num[10];
    int_to_str(level, level_num);
    int_to_str(score, score_num);

    const char* level_label = "LEVEL: ";
    const char* score_label = "SCORE: ";

    char level_text[22];
    char score_text[22];
    char level_buf[22];
    char score_buf[22];

    // Build score_text = "SCORE: xxx"
    int si = 0;
    while (score_label[si]) {
        score_text[si] = score_label[si];
        si++;
    }
    int sj = 0;
    while (score_num[sj]) {
        score_text[si++] = score_num[sj++];
    }
    score_text[si] = '\0';
    int score_len = si;

    // Build level_text = "LEVEL: x"
    int li = 0;
    while (level_label[li]) {
        level_text[li] = level_label[li];
        li++;
    }
    int lj = 0;
    while (level_num[lj]) {
        level_text[li++] = level_num[lj++];
    }
    level_text[li] = '\0';
    int level_len = li;

    // Fill score_buf with spaces
    int i;
    for (i = 0; i < 21; i++) score_buf[i] = ' ';
    score_buf[21] = '\0';

    // Fill level_buf with spaces
    for (i = 0; i < 21; i++) level_buf[i] = ' ';
    level_buf[21] = '\0';

    // Center align score_text
    int score_pad = (21 - score_len) / 2;
    for (i = 0; i < score_len; i++) {
        score_buf[score_pad + i] = score_text[i];
    }

    // Center align level_text
    int level_pad = (21 - level_len) / 2;
    for (i = 0; i < level_len; i++) {
        level_buf[level_pad + i] = level_text[i];
    }

    draw_string(0, 4, level_buf);
    draw_string(0, 5, score_buf);
}

// MATRIX INIT
void init_led_matrix(void) {
    char setup1[] = { 0x21 };  // Turn on oscillator
    char setup2[] = { 0x81 };  // Display on, blink off
    char setup3[] = { 0xE1 };  // Brightness: max

    i2c_write(LED_MATRIX_DEVICE_ID, setup1, 1);
    __delay_cycles(10000);
    i2c_write(LED_MATRIX_DEVICE_ID, setup2, 1);
    __delay_cycles(10000);
    i2c_write(LED_MATRIX_DEVICE_ID, setup3, 1);
    __delay_cycles(10000);
}

// CLEAR MATRIX
void clear_matrix(void) {
    char data[17];
    data[0] = 0x00; 
    int i;
    for (i = 1; i <= 16; i++) {
        data[i] = 0x00;
    }
    i2c_write(LED_MATRIX_DEVICE_ID, data, 17);
}

void update_matrix(void) {
    char data[17];
    data[0] = 0x00;
    int row, col;

    int i;
    for (i = 1; i <= 16; i++) data[i] = 0x00;

    for (row = 0; row < 16; row++) {
        for (col = 0; col < 8; col++) {
            if (game_field[row][col]) {
                int index = (row < 8) ? (2 * col + 1) : (2 * col + 2);
                data[index] |= (1 << (row % 8));
            }
        }
    }

    i2c_write(LED_MATRIX_DEVICE_ID, data, 17);
}

void spawn_new_block(void) {
    active_row = 15;  
    active_col = 3;
    block_active = 1;
    game_field[active_row][active_col] = 1;
    update_matrix();
}

void drop_block(void) {
    // If no active block, check for top collision before spawning a new one
    if (!block_active) {
        if (game_field[15][3]) {
            block_active = 0;
            game_state = STATE_GAME_OVER;
            return;
        }
        spawn_new_block();
        return;
    }

    // Handle left movement
    if (left_flag && active_col > 0 && !game_field[active_row][active_col - 1]) {
        game_field[active_row][active_col] = 0;
        active_col--;
        game_field[active_row][active_col] = 1;
        left_flag = 0;
    }

    // Handle right movement
    else if (right_flag && active_col < 7 && !game_field[active_row][active_col + 1]) {
        game_field[active_row][active_col] = 0;
        active_col++;
        game_field[active_row][active_col] = 1;
        right_flag = 0;
    }

    // If block cannot move down, lock it and check for row clears
    if (active_row == 0 || game_field[active_row - 1][active_col]) {
        block_active = 0;
        check_and_clear_rows();
        return;
    }

    // Move block down by one row
    game_field[active_row][active_col] = 0;
    active_row--;
    game_field[active_row][active_col] = 1;

    update_matrix();
}


void clear_game_field(void) {
    int r, c;
    for (r = 0; r < 16; r++) {
        for (c = 0; c < 8; c++) {
            game_field[r][c] = 0;
        }
    }
}

void check_and_clear_rows(void) {
    int row, col;
    int cleared = 0;

    for (row = 0; row < 16; row++) {
        int full = 1;
        for (col = 0; col < 8; col++) {
            if (!game_field[row][col]) {
                full = 0;
                break;
            }
        }

        if (full) {
            int r;
            for (r = row; r < 15; r++) {
                for (col = 0; col < 8; col++) {
                    game_field[r][col] = game_field[r + 1][col];
                }
            }
            for (col = 0; col < 8; col++) game_field[15][col] = 0;
            cleared++;
            row--;  // recheck this row
        }
    }

    if (cleared > 0) {
        rows_cleared += cleared;

        if (rows_cleared >= 10) level = 4;
        else if (rows_cleared >= 6) level = 3;
        else if (rows_cleared >= 3) level = 2;
        else level = 1;

        switch (level) {
            case 1: score += 5 * cleared; TB0CCR0 = 10000; break;
            case 2: score += 10 * cleared; TB0CCR0 = 8500; break;
            case 3: score += 15 * cleared; TB0CCR0 = 7000; break;
            case 4: score += 30 * cleared; TB0CCR0 = 5500; break;
        }

        update_matrix();
    }
}


// LED SCREEN I2C ISR
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

// LED MATRIX I2C ISR
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

#pragma vector = TIMER0_B0_VECTOR
__interrupt void Pattern_Transition_ISR(void) {
    if (game_state == STATE_PLAYING) {
        drop_block();
    }
    TB0CCTL0 &= ~ CCIFG;           
}

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
    pb_flag = 1;
    P1IFG &= ~BIT1;
}

