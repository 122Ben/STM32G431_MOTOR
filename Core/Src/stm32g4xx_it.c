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

/**
  * @brief  MemManage/BusFault/UsageFault are NOT individually enabled (SCB->SHCSR
  *         MEMFAULTENA/BUSFAULTENA/USGFAULTENA are 0 at reset and this project never
  *         sets them), so every fault currently escalates straight to HardFault -
  *         CFSR still tells us which one it really was (MMFSR/BFSR/UFSR sub-fields).
  *         Dumps PC/LR from the stacked exception frame plus the fault status/address
  *         registers over RTT before killing PWM and hanging, so a real fault is
  *         actually diagnosable instead of just a silent freeze.
  */
void HardFault_Handler_C(uint32_t *pStackFrame)
{
  /* pStackFrame[0..7] = R0, R1, R2, R3, R12, LR, PC, xPSR (auto-stacked by the CPU) */
  TIM1->BDTR &= ~TIM_BDTR_MOE;
  SEGGER_RTT_printf(0,
                     "*** HardFault *** PC=0x%08x LR=0x%08x xPSR=0x%08x\r\n"
                     "    CFSR=0x%08x HFSR=0x%08x MMFAR=0x%08x BFAR=0x%08x\r\n",
                     pStackFrame[6], pStackFrame[5], pStackFrame[7],
                     SCB->CFSR, SCB->HFSR, SCB->MMFAR, SCB->BFAR);
  while (1) { }
}

__attribute__((naked)) void HardFault_Handler(void)
{
  __asm volatile
  (
    "tst   lr, #4                \n"
    "ite   eq                    \n"
    "mrseq r0, msp                \n"
    "mrsne r0, psp                \n"
    "b     HardFault_Handler_C    \n"
  );
}

void MemManage_Handler(void)
{
  TIM1->BDTR &= ~TIM_BDTR_MOE;
  SEGGER_RTT_printf(0, "*** MemManage_Handler *** CFSR=0x%08x MMFAR=0x%08x\r\n", SCB->CFSR, SCB->MMFAR);
  while (1) { }
}

void BusFault_Handler(void)
{
  TIM1->BDTR &= ~TIM_BDTR_MOE;
  SEGGER_RTT_printf(0, "*** BusFault_Handler *** CFSR=0x%08x BFAR=0x%08x\r\n", SCB->CFSR, SCB->BFAR);
  while (1) { }
}

void UsageFault_Handler(void)
{
  TIM1->BDTR &= ~TIM_BDTR_MOE;
  SEGGER_RTT_printf(0, "*** UsageFault_Handler *** CFSR=0x%08x\r\n", SCB->CFSR);
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
