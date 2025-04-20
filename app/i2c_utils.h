#ifndef I2C_UTILS_H
#define I2C_UTILS_H

void i2c_write(int device, char data[], int len);
void init_i2c_screen(void);
void init_i2c_matrix(void);

#endif