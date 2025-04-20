#include <msp430.h>
#include "i2c_utils.h"
#include "led_screen.h"

#define MAX_I2C_LEN 64

char Screen_Data_Buffer[MAX_I2C_LEN];
int Screen_Data_Len = 0;
int Screen_Data_Cnt = 0;

char Matrix_Data_Buffer[MAX_I2C_LEN];
int Matrix_Data_Len = 0;
int Matrix_Data_Cnt = 0;

int screen = 0;
int matrix = 1;

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;
    PM5CTL0 &= ~LOCKLPM5;

    init_i2c_screen();
    init_i2c_matrix();
    init_led_screen();
    clear_display();

    display_score(100);

    while (1) {
        __no_operation();
    }
}

void display_start_game() {
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

void display_score(int score) {
    clear_screen();
    draw_string(0, 1, "     MINI TETRIS     ");
    draw_string(0, 3, "       SCORE:        ");

    char buffer[22];
    int i = 0;

    if (score >= 100) buffer[i++] = (score / 100) + '0';
    else buffer[i++] = ' ';
    if (score >= 10) buffer[i++] = ((score / 10) % 10) + '0';
    else buffer[i++] = ' ';
    buffer[i++] = (score % 10) + '0';

    buffer[i++] = ' ';
    buffer[i++] = 'P';
    buffer[i++] = 'O';
    buffer[i++] = 'I';
    buffer[i++] = 'N';
    buffer[i++] = 'T';
    buffer[i++] = 'S';

    while (i < 21) buffer[i++] = ' ';
    buffer[i] = '\0';

    draw_string(0, 5, buffer);
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
