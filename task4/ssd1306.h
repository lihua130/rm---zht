#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>

void ssd1306_init(void);
void ssd1306_show_waiting(void);
void ssd1306_show_coordinates(uint16_t x, uint16_t y);

#endif
