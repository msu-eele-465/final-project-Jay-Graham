#ifndef I2C_UTILS_H
#define I2C_UTILS_H

#define MAX_I2C_LEN 64

extern char Screen_Data_Buffer[MAX_I2C_LEN];
extern int Screen_Data_Len;
extern int Screen_Data_Cnt;

extern char Matrix_Data_Buffer[MAX_I2C_LEN];
extern int Matrix_Data_Len;
extern int Matrix_Data_Cnt;

void i2c_write(int device, char data[], int len);
void init_i2c_screen(void);
void init_i2c_matrix(void);

#endif
