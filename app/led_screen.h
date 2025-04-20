#ifndef LED_SCREEN_H
#define LED_SCREEN_H

void init_led_screen(void);
void clear_screen(void);
void draw_string(int col, int page, const char *str);

#endif