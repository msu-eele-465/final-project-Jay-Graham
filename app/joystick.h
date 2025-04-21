#ifndef JOYSTICK_H
#define JOYSTICK_H

// Joystick direction values
#define CENTER 0
#define LEFT   1
#define RIGHT  2

void init_joystick(void);
int get_direction(void);
int check_rotate(void);

#endif
