/**
  ******************************************************************************
  * @file    button.c
  * @brief   Debounced start/stop + long-press button on PC10 (active low,
  *          pressed = pulled to GND). The EXTI falling-edge callback only
  *          marks "a press started"; actual debounce and short/long
  *          classification happens by polling the pin level from the main
  *          loop, timed against the press-start tick.
  ******************************************************************************
  */

#include "button.h"

static volatile uint32_t s_pressStartTick = 0;
static volatile uint8_t  s_pressActive = 0;

void Button_Init(void)
{
  s_pressActive = 0;
}

void Button_EXTI_Callback(uint16_t GPIO_Pin)
{
  if ((GPIO_Pin == BUTTON_Pin) && (s_pressActive == 0U))
  {
    s_pressStartTick = HAL_GetTick();
    s_pressActive = 1U;
  }
}

ButtonEvent_t Button_Poll(void)
{
  uint32_t elapsed;
  GPIO_PinState level;

  if (!s_pressActive)
  {
    return BUTTON_EVENT_NONE;
  }

  elapsed = HAL_GetTick() - s_pressStartTick;
  if (elapsed < BUTTON_DEBOUNCE_MS)
  {
    return BUTTON_EVENT_NONE; /* still settling */
  }

  level = HAL_GPIO_ReadPin(BUTTON_GPIO_Port, BUTTON_Pin);
  if (level == GPIO_PIN_RESET)
  {
    return BUTTON_EVENT_NONE; /* still held down, wait for release */
  }

  s_pressActive = 0U;
  return (elapsed >= BUTTON_LONGPRESS_MS) ? BUTTON_EVENT_LONG_PRESS : BUTTON_EVENT_SHORT_PRESS;
}
