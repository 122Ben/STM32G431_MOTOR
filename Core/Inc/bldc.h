#ifndef __BLDC_H
#define __BLDC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

typedef enum
{
  BLDC_STOPPED = 0,
  BLDC_RUNNING,
  BLDC_FAULT
} BLDC_State_t;

void BLDC_Init(void);
void BLDC_Start(void);
void BLDC_Stop(void);
/* Fastest possible output shutdown (clears TIM1 MOE directly) - safe to call
 * from any context, including from inside protection checks in the main loop. */
void BLDC_EmergencyStop(void);
void BLDC_ClearFault(void);

/* duty in 0..PWM_DUTY_MAX (0.1% steps) */
void BLDC_SetDutyPermille(uint16_t duty);

BLDC_State_t BLDC_GetState(void);
uint8_t      BLDC_GetHallState(void);     /* 1..6 valid, 0 = invalid/unknown */
uint32_t     BLDC_GetStepIntervalUs(void);/* time between the last two commutations, in us */

/* Called from the Hall GPIO EXTI ISR (stm32g4xx_it.c) for pins HALL1/2/3. */
void BLDC_HallEXTI_Callback(uint16_t GPIO_Pin);

#ifdef __cplusplus
}
#endif

#endif /* __BLDC_H */
