/**
  ******************************************************************************
  * @file    opamp.c
  * @brief   OPAMP1/2/3 used as the three phase-current shunt amplifiers.
  *          Standalone mode is used (VINP/VINM both external pins, gain set by
  *          the external resistor network on the board, which is unknown here)
  *          with InternalOutput=ENABLE so the amplifier output is routed
  *          directly to the ADC (no external GPIO pin needed for VOUT):
  *            OPAMP1: VINP0=PA1, VINM0=PA3 -> internally to ADC1_IN3
  *            OPAMP2: VINP0=PA7, VINM0=PA5 -> internally to ADC2_IN3
  *            OPAMP3: VINP0=PB0, VINM0=PB2 -> internally to ADC1_IN12
  ******************************************************************************
  */

#include "opamp.h"

OPAMP_HandleTypeDef hopamp1;
OPAMP_HandleTypeDef hopamp2;
OPAMP_HandleTypeDef hopamp3;

static void OPAMP_CommonInit(OPAMP_HandleTypeDef *hopamp, OPAMP_TypeDef *instance)
{
  hopamp->Instance                     = instance;
  hopamp->Init.PowerMode                = OPAMP_POWERMODE_NORMALSPEED;
  hopamp->Init.Mode                     = OPAMP_STANDALONE_MODE;
  hopamp->Init.InvertingInput           = OPAMP_INVERTINGINPUT_IO0;
  hopamp->Init.NonInvertingInput        = OPAMP_NONINVERTINGINPUT_IO0;
  hopamp->Init.InternalOutput           = ENABLE;
  hopamp->Init.TimerControlledMuxmode   = OPAMP_TIMERCONTROLLEDMUXMODE_DISABLE;
  hopamp->Init.UserTrimming             = OPAMP_TRIMMING_FACTORY;
  if (HAL_OPAMP_Init(hopamp) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_OPAMP_Start(hopamp) != HAL_OK)
  {
    Error_Handler();
  }
}

void MX_OPAMP1_Init(void)
{
  OPAMP_CommonInit(&hopamp1, OPAMP1);
}

void MX_OPAMP2_Init(void)
{
  OPAMP_CommonInit(&hopamp2, OPAMP2);
}

void MX_OPAMP3_Init(void)
{
  OPAMP_CommonInit(&hopamp3, OPAMP3);
}
