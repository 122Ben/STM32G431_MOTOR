/**
  ******************************************************************************
  * @file    main.c
  * @brief   STM32G431 six-step (Hall-commutated) BLDC motor control.
  *          See README.md for the pin map, bring-up checklist and UART protocol.
  ******************************************************************************
  */

#include "main.h"
#include "gpio.h"
#include "tim.h"
#include "adc.h"
#include "opamp.h"
#include "usart.h"
#include "bldc.h"
#include "speed_pid.h"
#include "protection.h"
#include "button.h"
#include "uart_protocol.h"

static void MX_Peripherals_Init(void);
static void HandleButtonEvent(ButtonEvent_t ev);

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_Peripherals_Init();

  BLDC_Init();
  SpeedPID_Init();
  Protection_Init();
  Button_Init();
  UART_Protocol_Init();

  uint32_t lastAdcTick = 0;
  uint32_t lastPidTick = 0;

  while (1)
  {
    uint32_t now = HAL_GetTick();

    if ((now - lastAdcTick) >= 1U)
    {
      lastAdcTick = now;
      ADC_SampleAll();
      Protection_Check();
    }

    if ((BLDC_GetState() == BLDC_RUNNING) && (UART_Protocol_IsOpenLoop() == 0U) &&
        ((now - lastPidTick) >= SPEED_CONTROL_PERIOD_MS))
    {
      lastPidTick = now;
      SpeedPID_Update();
    }

    HandleButtonEvent(Button_Poll());

    UART_Protocol_Process();
    UART_Protocol_TelemetryTick();
  }
}

/* Long-press cycles through these RPM presets (0 = stop). */
static const uint32_t s_rpmPresets[] = { 1000U, 2000U, 3000U, 4000U, 5000U, 0U };
#define RPM_PRESET_COUNT (sizeof(s_rpmPresets) / sizeof(s_rpmPresets[0]))
static uint8_t s_presetIndex = 0U;

static void HandleButtonEvent(ButtonEvent_t ev)
{
  if (ev == BUTTON_EVENT_SHORT_PRESS)
  {
    if (BLDC_GetState() == BLDC_RUNNING)
    {
      BLDC_Stop();
    }
    else
    {
      BLDC_ClearFault();
      Protection_ClearFault();
      SpeedPID_Reset();
      BLDC_Start();
    }
  }
  else if (ev == BUTTON_EVENT_LONG_PRESS)
  {
    s_presetIndex = (uint8_t)((s_presetIndex + 1U) % RPM_PRESET_COUNT);
    SpeedPID_SetTargetRPM(s_rpmPresets[s_presetIndex]);
  }
}

static void MX_Peripherals_Init(void)
{
  MX_GPIO_Init();
  MX_TIM1_Init();
  MX_TIM7_Init();
  MX_ADC1_Init();
  MX_ADC2_Init();
  MX_OPAMP1_Init();
  MX_OPAMP2_Init();
  MX_OPAMP3_Init();
  MX_USART2_UART_Init();
}

/**
  * @brief  170MHz SYSCLK from HSI16 (no external crystal on this board):
  *         HSI16 /PLLM(4) = 4MHz *PLLN(85) = 340MHz VCO /PLLR(2) = 170MHz.
  *         AHB/APB1/APB2 all undivided (also 170MHz, within spec).
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState       = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM       = RCC_PLLM_DIV4;
  RCC_OscInitStruct.PLL.PLLN       = 85;
  RCC_OscInitStruct.PLL.PLLP       = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ       = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR       = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                                 RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief  Fatal init-time error: make sure the bridge cannot be driven, then hang.
  */
void Error_Handler(void)
{
  __disable_irq();
  TIM1->BDTR &= ~TIM_BDTR_MOE;
  while (1) { }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  (void)file;
  (void)line;
}
#endif
