/**
  ******************************************************************************
  * @file    tim.c
  * @brief   TIM1: three-phase complementary center-aligned PWM used for the
  *          six-step commutation bridge (CH1/2/3 + CH1N/2N/3N, dead time).
  *          TIM7: free-running 1MHz timer, read by the Hall EXTI ISR to time
  *          commutation intervals for speed measurement.
  *
  *          NOTE: CCMR1/CCMR2 (OCxM) and CCER (CCxE/CCxNE) are intentionally
  *          left in a "PWM configured but all channels disabled" state here.
  *          bldc.c takes over direct register control of those two registers
  *          on every Hall commutation step - this is why TIM_LOCKLEVEL_OFF is
  *          used in the break/dead-time config below (a nonzero lock level
  *          would freeze those bits after the first configuration).
  ******************************************************************************
  */

#include "tim.h"

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim7;

void MX_TIM1_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  htim1.Instance               = TIM1;
  htim1.Init.Prescaler         = 0;
  htim1.Init.CounterMode       = TIM_COUNTERMODE_CENTERALIGNED1;
  htim1.Init.Period            = TIM1_PWM_PERIOD;
  htim1.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }

  sMasterConfig.MasterOutputTrigger  = TIM_TRGO_RESET;
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode      = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /* All 3 channels: PWM mode 1, start at 0% duty. bldc.c overwrites OCxM/CCxE
   * on every commutation step, this initial config only needs to be valid. */
  sConfigOC.OCMode       = TIM_OCMODE_PWM1;
  sConfigOC.Pulse        = 0;
  sConfigOC.OCPolarity   = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity  = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode   = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState  = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }

  sBreakDeadTimeConfig.OffStateRunMode  = TIM_OSSR_ENABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_ENABLE;
  sBreakDeadTimeConfig.LockLevel        = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime         = TIM1_DEADTIME_DTG;
  sBreakDeadTimeConfig.BreakState       = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity    = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.BreakFilter      = 0;
  sBreakDeadTimeConfig.BreakAFMode      = TIM_BREAK_AFMODE_INPUT;
  sBreakDeadTimeConfig.Break2State      = TIM_BREAK2_DISABLE;
  sBreakDeadTimeConfig.Break2Polarity   = TIM_BREAK2POLARITY_HIGH;
  sBreakDeadTimeConfig.Break2Filter     = 0;
  sBreakDeadTimeConfig.Break2AFMode     = TIM_BREAK_AFMODE_INPUT;
  sBreakDeadTimeConfig.AutomaticOutput  = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }

  HAL_TIM_MspPostInit(&htim1);

  /* Start the timer and the (still disabled, CCxE=0) PWM/complementary channels
   * once, at init time. bldc.c only ever toggles CCER/CCMR bits afterwards. */
  if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1) != HAL_OK) { Error_Handler(); }
  if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2) != HAL_OK) { Error_Handler(); }
  if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3) != HAL_OK) { Error_Handler(); }
  if (HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1) != HAL_OK) { Error_Handler(); }
  if (HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_2) != HAL_OK) { Error_Handler(); }
  if (HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_3) != HAL_OK) { Error_Handler(); }

  /* Bring the bridge to a safe floating state: all outputs off until
   * BLDC_Start() enables MOE and the Hall ISR starts driving CCER/CCMR. */
  TIM1->CCER &= ~(TIM_CCER_CC1E | TIM_CCER_CC1NE | TIM_CCER_CC2E | TIM_CCER_CC2NE |
                  TIM_CCER_CC3E | TIM_CCER_CC3NE);
  TIM1->BDTR &= ~TIM_BDTR_MOE;
}

/**
  * @brief  Free-running 1MHz (1us/tick) 16-bit timer, used only by reading
  *         TIM7->CNT from the Hall EXTI ISR - no interrupt is enabled.
  */
void MX_TIM7_Init(void)
{
  htim7.Instance               = TIM7;
  htim7.Init.Prescaler         = (uint32_t)(SYSCLK_FREQ_HZ / SPEED_TIMER_TICK_HZ) - 1U;
  htim7.Init.CounterMode       = TIM_COUNTERMODE_UP;
  htim7.Init.Period            = 0xFFFF;
  htim7.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
  htim7.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim7) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_Base_Start(&htim7) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief  TIM1 PWM/complementary-PWM pin routing (CubeMX "MspPostInit" style).
  *         AF numbers were cross-checked against ST's official pin database
  *         (not all pins are AF6: PC13/PB15 complementary outputs are AF4).
  */
void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  if (htim->Instance == TIM1)
  {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* TIM1_CH1/CH2/CH3 : PA8/PA9/PA10, AF6 */
    GPIO_InitStruct.Pin       = TIM1_CH1_Pin | TIM1_CH2_Pin | TIM1_CH3_Pin;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF6_TIM1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* TIM1_CH2N : PA12, AF6 */
    GPIO_InitStruct.Pin       = TIM1_CH2N_Pin;
    GPIO_InitStruct.Alternate = GPIO_AF6_TIM1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* TIM1_CH1N : PC13, AF4 */
    GPIO_InitStruct.Pin       = TIM1_CH1N_Pin;
    GPIO_InitStruct.Alternate = GPIO_AF4_TIM1;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* TIM1_CH3N : PB15, AF4 */
    GPIO_InitStruct.Pin       = TIM1_CH3N_Pin;
    GPIO_InitStruct.Alternate = GPIO_AF4_TIM1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  }
}
