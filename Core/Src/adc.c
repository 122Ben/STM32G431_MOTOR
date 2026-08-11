/**
  ******************************************************************************
  * @file    adc.c
  * @brief   ADC1 (VBUS + phase U/W current) and ADC2 (phase V current).
  *          Six-step trapezoidal control does not need FOC-grade simultaneous
  *          3-shunt sampling, so a simple periodic software-triggered sweep is
  *          used instead of DMA/injected-synchronized conversions.
  ******************************************************************************
  */

#include "adc.h"

ADC_HandleTypeDef hadc1;
ADC_HandleTypeDef hadc2;
volatile ADC_Readings_t g_adcReadings;

static void ADC_CommonClockConfig(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC12;
  PeriphClkInit.Adc12ClockSelection  = RCC_ADC12CLKSOURCE_SYSCLK;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

void MX_ADC1_Init(void)
{
  ADC_ChannelConfTypeDef sConfig = {0};

  ADC_CommonClockConfig();

  hadc1.Instance                   = ADC1;
  hadc1.Init.ClockPrescaler        = ADC_CLOCK_ASYNC_DIV4; /* SYSCLK(170MHz)/4 = 42.5MHz */
  hadc1.Init.Resolution            = ADC_RESOLUTION_12B;
  hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
  hadc1.Init.GainCompensation      = 0;
  hadc1.Init.ScanConvMode          = ADC_SCAN_ENABLE;
  hadc1.Init.EOCSelection          = ADC_EOC_SEQ_CONV;
  hadc1.Init.LowPowerAutoWait      = DISABLE;
  hadc1.Init.ContinuousConvMode    = DISABLE;
  hadc1.Init.NbrOfConversion       = 3;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.SamplingMode          = ADC_SAMPLING_MODE_NORMAL;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.Overrun               = ADC_OVR_DATA_OVERWRITTEN;
  hadc1.Init.OversamplingMode      = DISABLE;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  sConfig.Channel      = ADC_CHANNEL_1;   /* VBUS, PA0 */
  sConfig.Rank         = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_47CYCLES_5;
  sConfig.SingleDiff   = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset       = 0;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  sConfig.Channel = ADC_CHANNEL_3;        /* phase U current, OPAMP1_OUT internal */
  sConfig.Rank    = ADC_REGULAR_RANK_2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  sConfig.Channel = ADC_CHANNEL_12;       /* phase W current, OPAMP3_OUT internal */
  sConfig.Rank    = ADC_REGULAR_RANK_3;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED) != HAL_OK)
  {
    Error_Handler();
  }
}

void MX_ADC2_Init(void)
{
  ADC_ChannelConfTypeDef sConfig = {0};

  hadc2.Instance                   = ADC2;
  hadc2.Init.ClockPrescaler        = ADC_CLOCK_ASYNC_DIV4;
  hadc2.Init.Resolution            = ADC_RESOLUTION_12B;
  hadc2.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
  hadc2.Init.GainCompensation      = 0;
  hadc2.Init.ScanConvMode          = ADC_SCAN_DISABLE;
  hadc2.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
  hadc2.Init.LowPowerAutoWait      = DISABLE;
  hadc2.Init.ContinuousConvMode    = DISABLE;
  hadc2.Init.NbrOfConversion       = 1;
  hadc2.Init.DiscontinuousConvMode = DISABLE;
  hadc2.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
  hadc2.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc2.Init.SamplingMode          = ADC_SAMPLING_MODE_NORMAL;
  hadc2.Init.DMAContinuousRequests = DISABLE;
  hadc2.Init.Overrun               = ADC_OVR_DATA_OVERWRITTEN;
  hadc2.Init.OversamplingMode      = DISABLE;
  if (HAL_ADC_Init(&hadc2) != HAL_OK)
  {
    Error_Handler();
  }

  sConfig.Channel      = ADC_CHANNEL_3;   /* phase V current, OPAMP2_OUT internal */
  sConfig.Rank         = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_47CYCLES_5;
  sConfig.SingleDiff   = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset       = 0;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_ADCEx_Calibration_Start(&hadc2, ADC_SINGLE_ENDED) != HAL_OK)
  {
    Error_Handler();
  }
}

void ADC_SampleAll(void)
{
  if (HAL_ADC_Start(&hadc1) != HAL_OK)
  {
    return;
  }
  if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
  {
    g_adcReadings.vbus_raw = (uint16_t)HAL_ADC_GetValue(&hadc1);
  }
  if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
  {
    g_adcReadings.curr_u_raw = (uint16_t)HAL_ADC_GetValue(&hadc1);
  }
  if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
  {
    g_adcReadings.curr_w_raw = (uint16_t)HAL_ADC_GetValue(&hadc1);
  }
  HAL_ADC_Stop(&hadc1);

  if (HAL_ADC_Start(&hadc2) == HAL_OK)
  {
    if (HAL_ADC_PollForConversion(&hadc2, 10) == HAL_OK)
    {
      g_adcReadings.curr_v_raw = (uint16_t)HAL_ADC_GetValue(&hadc2);
    }
    HAL_ADC_Stop(&hadc2);
  }
}
