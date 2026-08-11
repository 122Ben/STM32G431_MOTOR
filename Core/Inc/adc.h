#ifndef __ADC_H
#define __ADC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/* ADC1: VBUS (IN1), phase U current via OPAMP1_OUT (IN3), phase W current via OPAMP3_OUT (IN12)
 * ADC2: phase V current via OPAMP2_OUT (IN3)
 * (Internal OPAMPx->ADC routing confirmed against ST reference material: OPAMP1_OUT=ADC1_IN3,
 *  OPAMP2_OUT=ADC2_IN3, OPAMP3_OUT=ADC1_IN12.) */
extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;

typedef struct
{
  uint16_t vbus_raw;
  uint16_t curr_u_raw;
  uint16_t curr_v_raw;
  uint16_t curr_w_raw;
} ADC_Readings_t;

extern volatile ADC_Readings_t g_adcReadings;

void MX_ADC1_Init(void);
void MX_ADC2_Init(void);

/* Blocking software-triggered sweep of all 4 channels. Call from the main
 * super-loop (not from an ISR) at ~1kHz - six-step protection does not need
 * ADC-DMA-grade timing. */
void ADC_SampleAll(void);

#ifdef __cplusplus
}
#endif

#endif /* __ADC_H */
