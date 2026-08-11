#ifndef __BUTTON_H
#define __BUTTON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

typedef enum
{
  BUTTON_EVENT_NONE = 0,
  BUTTON_EVENT_SHORT_PRESS,
  BUTTON_EVENT_LONG_PRESS
} ButtonEvent_t;

void Button_Init(void);
/* Called from the Button GPIO EXTI ISR (stm32g4xx_it.c), pin==BUTTON_Pin only. */
void Button_EXTI_Callback(uint16_t GPIO_Pin);
/* Call every main-loop iteration; returns a one-shot event on release. */
ButtonEvent_t Button_Poll(void);

#ifdef __cplusplus
}
#endif

#endif /* __BUTTON_H */
