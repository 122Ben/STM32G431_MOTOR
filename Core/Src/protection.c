/**
  ******************************************************************************
  * @file    protection.c
  * @brief   VBUS under/over-voltage and per-phase overcurrent checks against
  *          the raw ADC readings sampled by adc.c. Thresholds are placeholder
  *          values in motor_config.h - calibrate CURRENT_ADC_ZERO,
  *          CURRENT_ADC_TRIP_DELTA and VBUS_DIVIDER_RATIO against the real
  *          board before relying on this for hardware protection.
  ******************************************************************************
  */

#include "protection.h"
#include "adc.h"
#include "bldc.h"
#include "SEGGER_RTT.h"

static volatile FaultCode_t s_lastFault = FAULT_NONE;

void Protection_Init(void)
{
  s_lastFault = FAULT_NONE;
}

static int32_t AbsCurrentDelta(uint16_t raw)
{
  int32_t delta = (int32_t)raw - (int32_t)CURRENT_ADC_ZERO;
  return (delta < 0) ? -delta : delta;
}

void Protection_Check(void)
{
  float vbus;

  if (s_lastFault != FAULT_NONE)
  {
    return; /* already latched, wait for Protection_ClearFault() */
  }

  vbus = Protection_GetVbusVoltage();
  if (vbus > VBUS_OVERVOLTAGE_V)
  {
    s_lastFault = FAULT_OVERVOLTAGE;
    SEGGER_RTT_WriteString(0, "[FAULT] overvoltage\r\n");
  }
  else if ((BLDC_GetState() == BLDC_RUNNING) && (vbus < VBUS_UNDERVOLTAGE_V))
  {
    s_lastFault = FAULT_UNDERVOLTAGE;
    SEGGER_RTT_WriteString(0, "[FAULT] undervoltage\r\n");
  }
  else if (AbsCurrentDelta(g_adcReadings.curr_u_raw) > (int32_t)CURRENT_ADC_TRIP_DELTA)
  {
    s_lastFault = FAULT_OVERCURRENT_U;
    SEGGER_RTT_WriteString(0, "[FAULT] overcurrent U\r\n");
  }
  else if (AbsCurrentDelta(g_adcReadings.curr_v_raw) > (int32_t)CURRENT_ADC_TRIP_DELTA)
  {
    s_lastFault = FAULT_OVERCURRENT_V;
    SEGGER_RTT_WriteString(0, "[FAULT] overcurrent V\r\n");
  }
  else if (AbsCurrentDelta(g_adcReadings.curr_w_raw) > (int32_t)CURRENT_ADC_TRIP_DELTA)
  {
    s_lastFault = FAULT_OVERCURRENT_W;
    SEGGER_RTT_WriteString(0, "[FAULT] overcurrent W\r\n");
  }

  if (s_lastFault != FAULT_NONE)
  {
    BLDC_EmergencyStop();
  }
}

void Protection_ClearFault(void)
{
  s_lastFault = FAULT_NONE;
}

FaultCode_t Protection_GetLastFault(void)
{
  return s_lastFault;
}

float Protection_GetVbusVoltage(void)
{
  return ((float)g_adcReadings.vbus_raw * VREFP_VOLTAGE / 4095.0f) * VBUS_DIVIDER_RATIO;
}
