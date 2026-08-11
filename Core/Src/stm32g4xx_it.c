/**
  ******************************************************************************
  * @file    stm32g4xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  */

#include "main.h"
#include "stm32g4xx_it.h"
#include "bldc.h"
#include "button.h"
#include "SEGGER_RTT.h"

extern UART_HandleTypeDef huart2;

/* ===================== Cortex-M4 processor interrupts ===================== */

void NMI_Handler(void)
{
  while (1) { }
}

void HardFault_Handler(void)
{
  /* A hard fault while a motor could be spinning is not something to try to
   * recover from in software - the safest thing this handler can do is make
   * sure the bridge is off, then stop. RTT write is a plain memory copy (no
   * blocking I/O), safe to call from a fault handler. */
  TIM1->BDTR &= ~TIM_BDTR_MOE;
  SEGGER_RTT_WriteString(0, "*** HardFault_Handler ***\r\n");
  while (1) { }
}

void MemManage_Handler(void)
{
  TIM1->BDTR &= ~TIM_BDTR_MOE;
  SEGGER_RTT_WriteString(0, "*** MemManage_Handler ***\r\n");
  while (1) { }
}

void BusFault_Handler(void)
{
  TIM1->BDTR &= ~TIM_BDTR_MOE;
  SEGGER_RTT_WriteString(0, "*** BusFault_Handler ***\r\n");
  while (1) { }
}

void UsageFault_Handler(void)
{
  TIM1->BDTR &= ~TIM_BDTR_MOE;
  SEGGER_RTT_WriteString(0, "*** UsageFault_Handler ***\r\n");
  while (1) { }
}

void SVC_Handler(void)
{
}

void DebugMon_Handler(void)
{
}

void PendSV_Handler(void)
{
}

void SysTick_Handler(void)
{
  HAL_IncTick();
}

/* ===================== Peripheral interrupts ===================== */

/* Hall sensors HALL1=PB8, HALL2=PB7, HALL3=PB6 -> EXTI lines 8,7,6 (shared vector) */
void EXTI9_5_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(HALL1_Pin);
  HAL_GPIO_EXTI_IRQHandler(HALL2_Pin);
  HAL_GPIO_EXTI_IRQHandler(HALL3_Pin);
}

/* Button PC10 -> EXTI line 10 (shared vector) */
void EXTI15_10_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(BUTTON_Pin);
}

void USART2_IRQHandler(void)
{
  HAL_UART_IRQHandler(&huart2);
}

/**
  * @brief  Single dispatch point for every EXTI line used in this project.
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if ((GPIO_Pin == HALL1_Pin) || (GPIO_Pin == HALL2_Pin) || (GPIO_Pin == HALL3_Pin))
  {
    BLDC_HallEXTI_Callback(GPIO_Pin);
  }
  else if (GPIO_Pin == BUTTON_Pin)
  {
    Button_EXTI_Callback(GPIO_Pin);
  }
}
