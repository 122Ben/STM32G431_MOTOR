#include "hall.h"
#include "motor_pwm.h"
#include "motor_control.h"
#include "rtt_log.h"

static uint8_t hall_previous;
static uint32_t hall_edge_count;

uint8_t Hall_Read(void)
{
  uint8_t u = (HAL_GPIO_ReadPin(HALL_A_GPIO_Port, HALL_A_Pin) == GPIO_PIN_RESET) ? 1U : 0U;
  uint8_t v = (HAL_GPIO_ReadPin(HALL_B_GPIO_Port, HALL_B_Pin) == GPIO_PIN_RESET) ? 1U : 0U;
  uint8_t w = (HAL_GPIO_ReadPin(HALL_C_GPIO_Port, HALL_C_Pin) == GPIO_PIN_RESET) ? 1U : 0U;
  return (uint8_t)((u << 2) | (v << 1) | w);
}

void Hall_Init(void)
{
  hall_edge_count = 0U;
  hall_previous = Hall_Read();
  RTT_Log_Hall(hall_previous,
               (uint8_t)((hall_previous != 0U) && (hall_previous != 7U)),
               1U, hall_edge_count);
}

void Hall_OnEdge(uint16_t gpio_pin)
{
  uint8_t hall;
  uint8_t changed;
  uint8_t valid;
  uint8_t transition_valid;

  if ((gpio_pin != HALL_A_Pin) && (gpio_pin != HALL_B_Pin) &&
      (gpio_pin != HALL_C_Pin))
  {
    return;
  }

  hall = Hall_Read();
  if (hall == hall_previous)
  {
    return;
  }

  changed = (uint8_t)(hall ^ hall_previous);
  valid = (uint8_t)((hall != 0U) && (hall != 7U));
  transition_valid = (uint8_t)((changed == 1U) || (changed == 2U) || (changed == 4U));
  hall_edge_count++;

  if ((valid == 0U) || (transition_valid == 0U))
  {
    MotorPwm_AllOff();
  }

  RTT_Log_Hall(hall, valid, transition_valid, hall_edge_count);
  MotorControl_OnHallTransition(hall, valid, transition_valid);
  hall_previous = hall;
}
