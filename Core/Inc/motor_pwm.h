#ifndef MOTOR_PWM_H
#define MOTOR_PWM_H

#include "main.h"

typedef enum
{
  MOTOR_STEP_OFF = 0,
  MOTOR_STEP_UH_VL = 1,
  MOTOR_STEP_UH_WL,
  MOTOR_STEP_VH_WL,
  MOTOR_STEP_VH_UL,
  MOTOR_STEP_WH_UL,
  MOTOR_STEP_WH_VL
} MotorStep_t;

#define MOTOR_TEST_DUTY_PERMILLE 50U
#define MOTOR_MAX_DUTY_PERMILLE  100U

void MotorPwm_Init(void);
void MotorPwm_AllOff(void);
void MotorPwm_SetDutyPermille(uint16_t duty_permille);
void MotorPwm_ApplyStep(MotorStep_t step);
MotorStep_t MotorPwm_GetStep(void);

#endif
