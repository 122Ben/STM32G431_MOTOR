/**
  ******************************************************************************
  * @file    uart_protocol.c
  * @brief   Simple ASCII line protocol on USART2 - see uart_protocol.h for the
  *          command/telemetry format. RX is interrupt-driven byte-by-byte into
  *          a line buffer; parsing/telemetry TX happens from the main loop
  *          (UART_Protocol_Process() / UART_Protocol_TelemetryTick()), never
  *          from the ISR itself.
  *
  *          Formatting avoids printf-family float support on purpose (keeps
  *          the Keil "use MicroLIB" choice irrelevant to this file).
  ******************************************************************************
  */

#include <string.h>
#include <stdio.h>
#include "uart_protocol.h"
#include "usart.h"
#include "bldc.h"
#include "speed_pid.h"
#include "protection.h"
#include "adc.h"

static uint8_t  s_rxByte;
static char     s_lineBuf[UART_RX_LINE_MAX];
static volatile uint16_t s_lineLen = 0;
static volatile uint8_t  s_lineReady = 0;

static uint8_t  s_openLoop = 0;
static uint32_t s_lastTelemetryTick = 0;

static void ArmReceive(void)
{
  HAL_UART_Receive_IT(&huart2, &s_rxByte, 1);
}

void UART_Protocol_Init(void)
{
  s_lineLen = 0;
  s_lineReady = 0;
  s_openLoop = 0;
  s_lastTelemetryTick = 0;
  ArmReceive();
}

uint8_t UART_Protocol_IsOpenLoop(void)
{
  return s_openLoop;
}

/**
  * @brief  HAL weak callback override: one byte has been received on any UART.
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance != USART2)
  {
    return;
  }

  if ((s_rxByte == '\n') || (s_rxByte == '\r'))
  {
    if (s_lineLen > 0U)
    {
      s_lineBuf[s_lineLen] = '\0';
      s_lineReady = 1U;
    }
  }
  else if (s_lineLen < (UART_RX_LINE_MAX - 1U))
  {
    s_lineBuf[s_lineLen++] = (char)s_rxByte;
  }
  else
  {
    s_lineLen = 0U; /* overflow: drop the line */
  }

  ArmReceive();
}

static uint32_t ParseUInt(const char *s)
{
  uint32_t v = 0U;
  while ((*s >= '0') && (*s <= '9'))
  {
    v = (v * 10U) + (uint32_t)(*s - '0');
    s++;
  }
  return v;
}

static void HandleLine(const char *line)
{
  if (line[0] == '\0')
  {
    return;
  }

  switch (line[0])
  {
    case 'S': case 's':
      BLDC_ClearFault();
      SpeedPID_Reset();
      BLDC_Start();
      break;

    case 'X': case 'x':
      BLDC_Stop();
      break;

    case 'C': case 'c':
      BLDC_ClearFault();
      Protection_ClearFault();
      break;

    case 'R': case 'r':
      s_openLoop = 0U;
      SpeedPID_SetTargetRPM(ParseUInt(&line[1]));
      break;

    case 'D': case 'd':
      s_openLoop = 1U;
      BLDC_SetDutyPermille((uint16_t)ParseUInt(&line[1]));
      break;

    case '?':
      s_lastTelemetryTick = 0U; /* force immediate telemetry on next tick */
      break;

    default:
      break; /* unknown command, ignore */
  }
}

void UART_Protocol_Process(void)
{
  if (s_lineReady)
  {
    HandleLine(s_lineBuf);
    s_lineLen = 0U;
    s_lineReady = 0U;
  }
}

static const char *StateName(BLDC_State_t st)
{
  switch (st)
  {
    case BLDC_RUNNING: return "RUNNING";
    case BLDC_FAULT:   return "FAULT";
    default:           return "STOPPED";
  }
}

static const char *FaultName(FaultCode_t f)
{
  switch (f)
  {
    case FAULT_OVERCURRENT_U: return "OVERCURRENT_U";
    case FAULT_OVERCURRENT_V: return "OVERCURRENT_V";
    case FAULT_OVERCURRENT_W: return "OVERCURRENT_W";
    case FAULT_OVERVOLTAGE:   return "OVERVOLTAGE";
    case FAULT_UNDERVOLTAGE:  return "UNDERVOLTAGE";
    case FAULT_HALL_INVALID:  return "HALL_INVALID";
    default:                  return "NONE";
  }
}

static void SendTelemetryLine(void)
{
  char line[128];
  uint16_t idx = 0U;
  uint32_t vbus_mV;

  vbus_mV = (uint32_t)(Protection_GetVbusVoltage() * 1000.0f);

  idx += (uint16_t)sprintf(&line[idx], "T VBUS=%lu.%03lumV IU=%u IV=%u IW=%u RPM=%lu TARGET=%lu STATE=%s FAULT=%s\r\n",
                            (unsigned long)(vbus_mV / 1000U), (unsigned long)(vbus_mV % 1000U),
                            (unsigned)g_adcReadings.curr_u_raw, (unsigned)g_adcReadings.curr_v_raw,
                            (unsigned)g_adcReadings.curr_w_raw,
                            (unsigned long)SpeedPID_GetMeasuredRPM(), (unsigned long)SpeedPID_GetTargetRPM(),
                            StateName(BLDC_GetState()), FaultName(Protection_GetLastFault()));

  HAL_UART_Transmit(&huart2, (uint8_t *)line, idx, 50);
}

void UART_Protocol_TelemetryTick(void)
{
  uint32_t now = HAL_GetTick();

  if ((now - s_lastTelemetryTick) >= TELEMETRY_PERIOD_MS)
  {
    s_lastTelemetryTick = now;
    SendTelemetryLine();
  }
}
