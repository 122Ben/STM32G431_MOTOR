#include "hall.h"
#include "motor_pwm.h"
#include "motor_control.h"
#include "rtt_log.h"

static uint8_t hall_previous;
static volatile uint32_t hall_edge_count;
static volatile uint32_t hall_rpm;
static uint32_t hall_last_cycle;
static uint32_t hall_period_cycles;

#define HALL_EDGES_PER_REV 24U
#define HALL_TIMER_HZ      168000000U

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
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0U;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
  Hall_ResetSpeed();
  hall_previous = Hall_Read();
  RTT_Log_Hall(hall_previous,
               (uint8_t)((hall_previous != 0U) && (hall_previous != 7U)),
               1U, hall_edge_count);
}

void Hall_ResetSpeed(void)
{
  hall_rpm = 0U;
  hall_last_cycle = 0U;
  hall_period_cycles = 0U;
}

void Hall_OnEdge(uint16_t gpio_pin)
{
  uint8_t hall;
  uint8_t changed;
  uint8_t valid;
  uint8_t transition_valid;
  uint32_t now;
  uint32_t period;

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
    RTT_Log_Hall(hall, valid, transition_valid, hall_edge_count);
  }
  else
  {
    now = DWT->CYCCNT;
    if (hall_last_cycle != 0U)
    {
      period = now - hall_last_cycle;
      hall_period_cycles = (hall_period_cycles == 0U) ? period :
                           (hall_period_cycles * 3U + period) / 4U;
      hall_rpm = (uint32_t)(((uint64_t)60U * HALL_TIMER_HZ) /
                            ((uint64_t)HALL_EDGES_PER_REV * hall_period_cycles));
    }
    hall_last_cycle = now;
  }

  MotorControl_OnHallTransition(hall, valid, transition_valid);
  hall_previous = hall;
}

uint32_t Hall_GetEdgeCount(void)
{
  return hall_edge_count;
}

uint32_t Hall_GetRpm(void)
{
  return hall_rpm;
}
