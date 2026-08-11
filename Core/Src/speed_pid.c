/**
  ******************************************************************************
  * @file    speed_pid.c
  * @brief   RPM measurement from the Hall commutation interval (bldc.c/TIM7)
  *          and a position-form PID that drives BLDC_SetDutyPermille().
  *
  *          One commutation step = 1/6 electrical revolution. With the step
  *          interval in microseconds:
  *            RPM = 60 * 1e6 / (interval_us * 6 * MOTOR_POLE_PAIRS)
  *                = 10 000 000 / (interval_us * MOTOR_POLE_PAIRS)
  ******************************************************************************
  */

#include "speed_pid.h"
#include "bldc.h"

static volatile uint32_t s_targetRpm = DEFAULT_TARGET_RPM;
static volatile uint32_t s_measuredRpm = 0;

static float s_integral;
static float s_prevError;

void SpeedPID_Init(void)
{
  s_targetRpm = DEFAULT_TARGET_RPM;
  s_measuredRpm = 0;
  SpeedPID_Reset();
}

void SpeedPID_Reset(void)
{
  s_integral = 0.0f;
  s_prevError = 0.0f;
}

void SpeedPID_SetTargetRPM(uint32_t rpm)
{
  if (rpm > MAX_TARGET_RPM)
  {
    rpm = MAX_TARGET_RPM;
  }
  s_targetRpm = rpm;
}

uint32_t SpeedPID_GetTargetRPM(void)
{
  return s_targetRpm;
}

uint32_t SpeedPID_GetMeasuredRPM(void)
{
  return s_measuredRpm;
}

static uint32_t ComputeMeasuredRpm(void)
{
  uint32_t intervalUs = BLDC_GetStepIntervalUs();

  /* Reject implausibly short intervals (glitch/noise) and "stopped" (0xFFFFFFFF). */
  if ((intervalUs < 20U) || (intervalUs >= 0x0FFFFFFFU))
  {
    return 0U;
  }
  return 10000000UL / (intervalUs * MOTOR_POLE_PAIRS);
}

void SpeedPID_Update(void)
{
  float error, dt, output;
  uint32_t duty;

  s_measuredRpm = ComputeMeasuredRpm();

  dt    = (float)SPEED_CONTROL_PERIOD_MS / 1000.0f;
  error = (float)s_targetRpm - (float)s_measuredRpm;

  output = (SPEED_PID_KP * error) + (SPEED_PID_KI * s_integral) +
           (SPEED_PID_KD * (error - s_prevError) / dt);

  /* Clamp + simple anti-windup: only keep integrating while not saturated
   * in the direction that would push the output further out of range. */
  if (output > SPEED_PID_OUT_MAX)
  {
    output = SPEED_PID_OUT_MAX;
    if (error > 0.0f) { s_integral += error * dt; }
  }
  else if (output < SPEED_PID_OUT_MIN)
  {
    output = SPEED_PID_OUT_MIN;
    if (error < 0.0f) { s_integral += error * dt; }
  }
  else
  {
    s_integral += error * dt;
  }

  s_prevError = error;

  duty = (uint32_t)output;
  if ((s_targetRpm > 0U) && (duty < PWM_DUTY_MIN_START))
  {
    duty = PWM_DUTY_MIN_START;
  }
  if (s_targetRpm == 0U)
  {
    duty = 0U;
  }

  BLDC_SetDutyPermille((uint16_t)duty);
}
