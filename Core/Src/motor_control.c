#include "motor_control.h"
#include "hall.h"
#include "main.h"
#include "motor_pwm.h"
#include "rtt_log.h"

/*
 * First candidate table for the measured forward Hall sequence:
 * 001 -> 011 -> 010 -> 110 -> 100 -> 101.
 * If the pump only vibrates or draws excessive current, stop immediately.
 * The table may need a cyclic phase shift; do not reorder the power stage.
 */
static const MotorStep_t hall_to_step[8] =
{
  MOTOR_STEP_OFF,    /* 000: invalid */
  MOTOR_STEP_UH_VL,  /* 001 */
  MOTOR_STEP_VH_WL,  /* 010 */
  MOTOR_STEP_UH_WL,  /* 011 */
  MOTOR_STEP_WH_UL,  /* 100 */
  MOTOR_STEP_WH_VL,  /* 101 */
  MOTOR_STEP_VH_UL,  /* 110 */
  MOTOR_STEP_OFF     /* 111: invalid */
};

static volatile uint8_t motor_running;
static volatile uint32_t last_hall_tick;
static uint32_t last_status_tick;

static uint8_t MotorControl_HallIsValid(uint8_t hall)
{
  return (uint8_t)((hall != 0U) && (hall != 7U));
}

void MotorControl_Init(void)
{
  motor_running = 0U;
  last_hall_tick = HAL_GetTick();
  last_status_tick = last_hall_tick;
  MotorPwm_SetDutyPermille(MOTOR_TEST_DUTY_PERMILLE);
  MotorPwm_AllOff();
  RTT_Log_String("Motor stopped; button toggles run/stop\r\n");
}

void MotorControl_Start(void)
{
  uint8_t hall = Hall_Read();

  if (MotorControl_HallIsValid(hall) == 0U)
  {
    MotorPwm_AllOff();
    RTT_Log_String("START rejected: invalid Hall\r\n");
    return;
  }

  last_hall_tick = HAL_GetTick();
  last_status_tick = last_hall_tick;
  Hall_ResetSpeed();
  motor_running = 1U;
  MotorPwm_ApplyStep(hall_to_step[hall]);
  HAL_GPIO_WritePin(STATUS_GPIO_Port, STATUS_Pin, GPIO_PIN_SET);
  RTT_Log_String("RUN duty=5%\r\n");
}

void MotorControl_Stop(void)
{
  motor_running = 0U;
  MotorPwm_AllOff();
  HAL_GPIO_WritePin(STATUS_GPIO_Port, STATUS_Pin, GPIO_PIN_RESET);
  RTT_Log_String("STOP\r\n");
}

void MotorControl_Toggle(void)
{
  if (motor_running != 0U)
  {
    MotorControl_Stop();
  }
  else
  {
    MotorControl_Start();
  }
}

void MotorControl_OnHallTransition(uint8_t hall, uint8_t valid,
                                   uint8_t transition_valid)
{
  if (motor_running == 0U)
  {
    return;
  }

  if ((valid == 0U) || (transition_valid == 0U))
  {
    motor_running = 0U;
    MotorPwm_AllOff();
    RTT_Log_String("FAULT Hall\r\n");
    return;
  }

  last_hall_tick = HAL_GetTick();
  MotorPwm_ApplyStep(hall_to_step[hall]);
}

void MotorControl_Process(void)
{
  uint32_t now = HAL_GetTick();

  if ((motor_running != 0U) &&
      ((now - last_hall_tick) > MOTOR_HALL_TIMEOUT_MS))
  {
    motor_running = 0U;
    MotorPwm_AllOff();
    HAL_GPIO_WritePin(STATUS_GPIO_Port, STATUS_Pin, GPIO_PIN_RESET);
    RTT_Log_String("FAULT Hall timeout\r\n");
  }
  else if ((motor_running != 0U) && ((now - last_status_tick) >= 200U))
  {
    last_status_tick = now;
    RTT_Log_Status(Hall_Read(), Hall_GetRpm(), Hall_GetEdgeCount());
  }
}

uint8_t MotorControl_IsRunning(void)
{
  return motor_running;
}
