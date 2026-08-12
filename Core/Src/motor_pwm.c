#include "motor_pwm.h"
#include "tim.h"

static uint16_t motor_duty_permille;
static MotorStep_t motor_step = MOTOR_STEP_OFF;

static void MotorPwm_SetLowSidesOff(void)
{
  HAL_GPIO_WritePin(UL_GPIO_Port, UL_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(VL_GPIO_Port, VL_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(WL_GPIO_Port, WL_Pin, GPIO_PIN_RESET);
}

static void MotorPwm_StopHighSides(void)
{
  (void)HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
  (void)HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_2);
  (void)HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_3);
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0U);
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0U);
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, 0U);
}

static void MotorPwm_StartHighSide(uint32_t channel)
{
  uint32_t compare = ((__HAL_TIM_GET_AUTORELOAD(&htim1) + 1U) *
                      motor_duty_permille) / 1000U;

  __HAL_TIM_SET_COMPARE(&htim1, channel, compare);
  (void)HAL_TIM_PWM_Start(&htim1, channel);
}

void MotorPwm_Init(void)
{
  MotorPwm_AllOff();
  MotorPwm_SetDutyPermille(MOTOR_TEST_DUTY_PERMILLE);
}

void MotorPwm_AllOff(void)
{
  /* Remove every gate command before applying any new commutation state. */
  MotorPwm_StopHighSides();
  MotorPwm_SetLowSidesOff();
  motor_step = MOTOR_STEP_OFF;
}

void MotorPwm_SetDutyPermille(uint16_t duty_permille)
{
  if (duty_permille > MOTOR_MAX_DUTY_PERMILLE)
  {
    duty_permille = MOTOR_MAX_DUTY_PERMILLE;
  }
  motor_duty_permille = duty_permille;
}

void MotorPwm_ApplyStep(MotorStep_t step)
{
  MotorPwm_AllOff();

  switch (step)
  {
    case MOTOR_STEP_UH_VL:
      HAL_GPIO_WritePin(VL_GPIO_Port, VL_Pin, GPIO_PIN_SET);
      MotorPwm_StartHighSide(TIM_CHANNEL_1);
      break;
    case MOTOR_STEP_UH_WL:
      HAL_GPIO_WritePin(WL_GPIO_Port, WL_Pin, GPIO_PIN_SET);
      MotorPwm_StartHighSide(TIM_CHANNEL_1);
      break;
    case MOTOR_STEP_VH_WL:
      HAL_GPIO_WritePin(WL_GPIO_Port, WL_Pin, GPIO_PIN_SET);
      MotorPwm_StartHighSide(TIM_CHANNEL_2);
      break;
    case MOTOR_STEP_VH_UL:
      HAL_GPIO_WritePin(UL_GPIO_Port, UL_Pin, GPIO_PIN_SET);
      MotorPwm_StartHighSide(TIM_CHANNEL_2);
      break;
    case MOTOR_STEP_WH_UL:
      HAL_GPIO_WritePin(UL_GPIO_Port, UL_Pin, GPIO_PIN_SET);
      MotorPwm_StartHighSide(TIM_CHANNEL_3);
      break;
    case MOTOR_STEP_WH_VL:
      HAL_GPIO_WritePin(VL_GPIO_Port, VL_Pin, GPIO_PIN_SET);
      MotorPwm_StartHighSide(TIM_CHANNEL_3);
      break;
    case MOTOR_STEP_OFF:
    default:
      step = MOTOR_STEP_OFF;
      break;
  }

  motor_step = step;
}

MotorStep_t MotorPwm_GetStep(void)
{
  return motor_step;
}
