#ifndef __TIM_H
#define __TIM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

extern TIM_HandleTypeDef htim1;   /* three-phase complementary PWM, commutation */
extern TIM_HandleTypeDef htim7;   /* free-running 1MHz timer for hall interval measurement */

void MX_TIM1_Init(void);
void MX_TIM7_Init(void);
void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

#ifdef __cplusplus
}
#endif

#endif /* __TIM_H */
