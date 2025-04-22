#include <msp430.h>
#include "i2c_utils.h"
#include "led_matrix.h"

#define LED_MATRIX_DEVICE_ID 1 

static const char matrix_init_cmds[][2] = {
    { 0x00, 0x21 },  
    { 0x00, 0x81 },  
    { 0x00, 0xEF }   
};

void init_led_matrix(void) {
    int i;
    for (i = 0; i < 3; i++) {
        i2c_write(LED_MATRIX_DEVICE_ID, (char *)matrix_init_cmds[i], 2);
    }
}

void clear_matrix(void) {
    char buffer[17];
    buffer[0] = 0x00;  
    int i;
    for (i = 1; i < 17; i++) {
        buffer[i] = 0x00; 
    }
    i2c_write(LED_MATRIX_DEVICE_ID, buffer, sizeof(buffer));
}

void set_row(int row, char col_mask) {
    if (row < 0 || row > 15) return;

    char buffer[2];
    buffer[0] = row * 2;   // HT16K33 uses even addresses for each row
    buffer[1] = col_mask;  // 8 bits for 8 columns in this row

    i2c_write(LED_MATRIX_DEVICE_ID, buffer, sizeof(buffer));
}