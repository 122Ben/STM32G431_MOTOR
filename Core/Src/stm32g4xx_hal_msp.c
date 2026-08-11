/**
  ******************************************************************************
  * @file    stm32g4xx_hal_msp.c
  * @brief   HAL MSP (peripheral clock enable + low-level GPIO/NVIC config),
  *          CubeMX style: one HAL_xxx_MspInit() per peripheral, dispatching on
  *          hxxx->Instance when a peripheral family has several instances.
  ******************************************************************************
  */

#include "main.h"

void HAL_MspInit(void)
{
  __HAL_RCC_SYSCFG_CLK_ENABLE();
  __HAL_RCC_PWR_CLK_ENABLE();

  HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
}

/* ===================== ADC ===================== */
void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  if (hadc->Instance == ADC1)
  {
    __HAL_RCC_ADC12_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* VBUS sense, PA0 -> ADC1_IN1, analog mode */
    GPIO_InitStruct.Pin  = VBUS_ADC_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(VBUS_ADC_GPIO_Port, &GPIO_InitStruct);
  }
  else if (hadc->Instance == ADC2)
  {
    __HAL_RCC_ADC12_CLK_ENABLE();
    /* ADC2's only input in this project is the internal OPAMP2 output - no GPIO to configure. */
  }
}

/* ===================== OPAMP ===================== */
void HAL_OPAMP_MspInit(OPAMP_HandleTypeDef *hopamp)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;

  if (hopamp->Instance == OPAMP1)
  {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitStruct.Pin = CURR1_OPAMP_VINP_Pin | CURR1_OPAMP_VINM_Pin; /* PA1, PA3 */
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  }
  else if (hopamp->Instance == OPAMP2)
  {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitStruct.Pin = CURR2_OPAMP_VINP_Pin | CURR2_OPAMP_VINM_Pin; /* PA7, PA5 */
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  }
  else if (hopamp->Instance == OPAMP3)
  {
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitStruct.Pin = CURR3_OPAMP_VINP_Pin | CURR3_OPAMP_VINM_Pin; /* PB0, PB2 */
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  }
}

/* ===================== TIM ===================== */
void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM1)
  {
    __HAL_RCC_TIM1_CLK_ENABLE();
    /* GPIO routing for TIM1 is done in HAL_TIM_MspPostInit() (tim.c), CubeMX style. */
  }
}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM7)
  {
    __HAL_RCC_TIM7_CLK_ENABLE();
  }
}

/* ===================== UART ===================== */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  if (huart->Instance == USART2)
  {
    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* USART2_TX=PB3, USART2_RX=PB4, AF7 */
    GPIO_InitStruct.Pin       = USART2_TX_Pin | USART2_RX_Pin;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(USART2_IRQn, 3, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
  }
}
