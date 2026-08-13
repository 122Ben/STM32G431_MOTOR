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

static volatile MotorControlState_t motor_state;
static volatile uint32_t last_hall_tick;
static uint32_t last_status_tick;
static uint32_t last_control_tick;
static uint32_t diag_start_tick;
static uint32_t diag_start_edges;
static uint8_t diag_start_hall;
static uint8_t diag_step;

static const MotorStep_t diagnostic_steps[6] =
{
  MOTOR_STEP_VH_WL,
  MOTOR_STEP_VH_UL,
  MOTOR_STEP_WH_UL,
  MOTOR_STEP_WH_VL,
  MOTOR_STEP_UH_VL,
  MOTOR_STEP_UH_WL
};

static uint8_t MotorControl_HallIsValid(uint8_t hall)
{
  return (uint8_t)((hall != 0U) && (hall != 7U));
}

void MotorControl_Init(void)
{
  motor_state = MOTOR_STATE_STOP;
  last_hall_tick = HAL_GetTick();
  last_status_tick = last_hall_tick;
  last_control_tick = last_hall_tick;
  diag_step = 0U;
  MotorPwm_SetDutyPermille(MOTOR_TEST_DUTY_PERMILLE);
  MotorPwm_AllOff();
#if MOTOR_DIAGNOSTIC_MODE
  RTT_Log_String("DIAG mode: button pulses one step at 5% for 50ms\r\n");
#else
  RTT_Log_String("Motor stopped; button toggles run/stop\r\n");
#endif
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
  motor_state = MOTOR_STATE_HALL_RUN;
  MotorPwm_ApplyStep(hall_to_step[hall]);
  HAL_GPIO_WritePin(STATUS_GPIO_Port, STATUS_Pin, GPIO_PIN_SET);
  RTT_Log_String("RUN duty=5%\r\n");
}

void MotorControl_Stop(void)
{
  motor_state = MOTOR_STATE_STOP;
  MotorPwm_AllOff();
  HAL_GPIO_WritePin(STATUS_GPIO_Port, STATUS_Pin, GPIO_PIN_RESET);
  RTT_Log_String("STOP\r\n");
}

void MotorControl_Toggle(void)
{
#if MOTOR_DIAGNOSTIC_MODE
  if (motor_state == MOTOR_STATE_DIAG_PULSE)
  {
    MotorControl_Stop();
    return;
  }

  diag_start_hall = Hall_Read();
  diag_start_edges = Hall_GetEdgeCount();
  diag_start_tick = HAL_GetTick();
  motor_state = MOTOR_STATE_DIAG_PULSE;
  MotorPwm_SetDutyPermille(MOTOR_TEST_DUTY_PERMILLE);
  MotorPwm_ApplyStep(diagnostic_steps[diag_step]);
  HAL_GPIO_WritePin(STATUS_GPIO_Port, STATUS_Pin, GPIO_PIN_SET);
#else
  if (motor_state == MOTOR_STATE_HALL_RUN)
  {
    MotorControl_Stop();
  }
  else
  {
    MotorControl_Start();
  }
#endif
}

void MotorControl_OnHallTransition(uint8_t hall, uint8_t valid,
                                   uint8_t transition_valid)
{
  if (motor_state != MOTOR_STATE_HALL_RUN)
  {
    return;
  }

  if ((valid == 0U) || (transition_valid == 0U))
  {
    motor_state = MOTOR_STATE_FAULT;
    MotorPwm_AllOff();
    HAL_GPIO_WritePin(STATUS_GPIO_Port, STATUS_Pin, GPIO_PIN_RESET);
    RTT_Log_String("FAULT Hall\r\n");
    return;
  }

  last_hall_tick = HAL_GetTick();
  MotorPwm_ApplyStep(hall_to_step[hall]);
}

void MotorControl_Process(void)
{
  uint32_t now = HAL_GetTick();

  if (now == last_control_tick)
  {
    return;
  }
  last_control_tick = now;

  if ((motor_state == MOTOR_STATE_DIAG_PULSE) &&
      ((now - diag_start_tick) >= MOTOR_DIAG_PULSE_MS))
  {
    uint8_t completed_step = diag_step;

    MotorPwm_AllOff();
    HAL_GPIO_WritePin(STATUS_GPIO_Port, STATUS_Pin, GPIO_PIN_RESET);
    motor_state = MOTOR_STATE_STOP;
    RTT_Log_Diagnostic((uint8_t)(completed_step + 1U), diag_start_hall,
                       Hall_Read(), Hall_GetEdgeCount() - diag_start_edges);
    diag_step = (uint8_t)((diag_step + 1U) % 6U);
    return;
  }

  if ((motor_state == MOTOR_STATE_HALL_RUN) &&
      ((now - last_hall_tick) > MOTOR_HALL_TIMEOUT_MS))
  {
    motor_state = MOTOR_STATE_FAULT;
    MotorPwm_AllOff();
    HAL_GPIO_WritePin(STATUS_GPIO_Port, STATUS_Pin, GPIO_PIN_RESET);
    RTT_Log_String("FAULT Hall timeout\r\n");
  }
  else if ((motor_state == MOTOR_STATE_HALL_RUN) &&
           ((now - last_status_tick) >= 200U))
  {
    last_status_tick = now;
    RTT_Log_Status(Hall_Read(), Hall_GetRpm(), Hall_GetEdgeCount());
  }
}

uint8_t MotorControl_IsRunning(void)
{
  return (uint8_t)(motor_state == MOTOR_STATE_HALL_RUN);
}

MotorControlState_t MotorControl_GetState(void)
{
  return motor_state;
}
