/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   Plain GPIO inputs: Hall sensors (PB6/PB7/PB8) and BUTTON (PC10).
  *          Peripheral alternate-function pins (TIM1, ADC, OPAMP, USART2) are
  *          configured in their own Msp init functions, CubeMX style.
  ******************************************************************************
  */

#include "gpio.h"

/**
  * @brief  Configure the Hall sensor inputs and the user button.
  *         Hall inputs: floating/open-drain hall sensors are assumed, so the
  *         internal pull-up is enabled - remove it if the board already has
  *         external pull-ups (double pull-up is harmless but check hall
  *         supply/output type on the real board).
  *         Button: assumed active-low (pressed = pulled to GND), internal
  *         pull-up enabled, falling edge interrupt.
  */
void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  /* Hall sensors: HALL1=PB8, HALL2=PB7, HALL3=PB6, both-edge interrupt for commutation */
  GPIO_InitStruct.Pin  = HALL1_Pin | HALL2_Pin | HALL3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* Button: PC10, active low, falling edge interrupt */
  GPIO_InitStruct.Pin  = BUTTON_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(BUTTON_GPIO_Port, &GPIO_InitStruct);

  /* Hall EXTI (lines 5..9 share EXTI9_5_IRQn): high priority, commutation is timing critical */
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  /* Button EXTI (lines 10..15 share EXTI15_10_IRQn): low priority */
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}
