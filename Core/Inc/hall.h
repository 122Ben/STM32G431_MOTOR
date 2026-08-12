#ifndef HALL_H
#define HALL_H

#include "main.h"

void Hall_Init(void);
uint8_t Hall_Read(void);
void Hall_OnEdge(uint16_t gpio_pin);

#endif
