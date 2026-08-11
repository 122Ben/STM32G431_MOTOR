/**
  ******************************************************************************
  * @file    main.h
  ******************************************************************************
  */

#ifndef MAIN_H
#define MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32g4xx_hal.h"
#include "motor_config.h"

void Error_Handler(void);
void SystemClock_Config(void);

/* ===================== Pin definitions (CubeMX style) ===================== */

/* TIM1 three-phase complementary PWM */
#define TIM1_CH1_Pin            GPIO_PIN_8
#define TIM1_CH1_GPIO_Port      GPIOA
#define TIM1_CH2_Pin            GPIO_PIN_9
#define TIM1_CH2_GPIO_Port      GPIOA
#define TIM1_CH3_Pin            GPIO_PIN_10
#define TIM1_CH3_GPIO_Port      GPIOA
#define TIM1_CH1N_Pin           GPIO_PIN_13
#define TIM1_CH1N_GPIO_Port     GPIOC
#define TIM1_CH2N_Pin           GPIO_PIN_12
#define TIM1_CH2N_GPIO_Port     GPIOA
#define TIM1_CH3N_Pin           GPIO_PIN_15
#define TIM1_CH3N_GPIO_Port     GPIOB

/* Hall sensors */
#define HALL1_Pin               GPIO_PIN_8
#define HALL1_GPIO_Port         GPIOB
#define HALL2_Pin               GPIO_PIN_7
#define HALL2_GPIO_Port         GPIOB
#define HALL3_Pin               GPIO_PIN_6
#define HALL3_GPIO_Port         GPIOB

/* VBUS sense */
#define VBUS_ADC_Pin             GPIO_PIN_0
#define VBUS_ADC_GPIO_Port       GPIOA

/* Button */
#define BUTTON_Pin               GPIO_PIN_10
#define BUTTON_GPIO_Port         GPIOC
#define BUTTON_EXTI_IRQn         EXTI15_10_IRQn

/* USART2 */
#define USART2_TX_Pin            GPIO_PIN_3
#define USART2_TX_GPIO_Port      GPIOB
#define USART2_RX_Pin            GPIO_PIN_4
#define USART2_RX_GPIO_Port      GPIOB

/* Current feedback op-amps: OPAMP1 (U), OPAMP2 (V), OPAMP3 (W) */
#define CURR1_OPAMP_VINP_Pin     GPIO_PIN_1
#define CURR1_OPAMP_VINP_GPIO_Port GPIOA
#define CURR1_OPAMP_VINM_Pin     GPIO_PIN_3
#define CURR1_OPAMP_VINM_GPIO_Port GPIOA
#define CURR2_OPAMP_VINP_Pin     GPIO_PIN_7
#define CURR2_OPAMP_VINP_GPIO_Port GPIOA
#define CURR2_OPAMP_VINM_Pin     GPIO_PIN_5
#define CURR2_OPAMP_VINM_GPIO_Port GPIOA
#define CURR3_OPAMP_VINP_Pin     GPIO_PIN_0
#define CURR3_OPAMP_VINP_GPIO_Port GPIOB
#define CURR3_OPAMP_VINM_Pin     GPIO_PIN_2
#define CURR3_OPAMP_VINM_GPIO_Port GPIOB

#ifdef __cplusplus
}
#endif

#endif /* MAIN_H */
