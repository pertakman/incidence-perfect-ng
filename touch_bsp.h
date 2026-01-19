#ifndef TOUCH_BSP_H
#define TOUCH_BSP_H

#include <Arduino.h>

// Touch BSP public API
void Touch_Init(void);
uint8_t getTouch(uint16_t *x, uint16_t *y);

#endif
