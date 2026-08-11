/**
  ******************************************************************************
  * @file    bldc.c
  * @brief   Six-step Hall-commutated BLDC bridge control on TIM1.
  *
  *          Hall inputs are read as a 3-bit code: bit0=HALL1(PB8), bit1=HALL2(PB7),
  *          bit2=HALL3(PB6). Only codes 1..6 are valid (0 and 7 mean a wiring
  *          fault / glitch and trigger an emergency stop).
  *
  *          Each valid Hall code selects a PhaseRole_t for 3 LOGICAL phases
  *          P1/P2/P3. P1 is always TIM1 channel 1. P2/P3 map to TIM1 channel
  *          2/3 normally, or are SWAPPED when MOTOR_HALL_TABLE_SELECT is set
  *          to MOTOR_HALL_TABLE_REVERSE. Swapping any two of the three motor
  *          phases is a textbook way to reverse the rotation direction of a
  *          3-phase machine, and it does not require guessing a second,
  *          independently-derived commutation table - which is important here
  *          since the real Hall-to-winding relationship of a hand-wired board
  *          is unknown until first bring-up.
  *
  *          Per phase, per role:
  *            PH_PWM : OCxM=PWM1,            CCxE=1, CCxNE=1 (synchronous rectification)
  *            PH_ON  : OCxM=FORCED_INACTIVE, CCxE=1, CCxNE=1 (low side forced on,
  *                     high side forced off - inactive level with OCxPolarity=HIGH)
  *            PH_OFF : CCxE=0, CCxNE=0 (both outputs disabled -> floating phase)
  ******************************************************************************
  */

#include "bldc.h"
#include "tim.h"

typedef enum { PH_OFF = 0, PH_PWM, PH_ON } PhaseRole_t;

typedef struct
{
  PhaseRole_t role[3]; /* [0]=P1 [1]=P2 [2]=P3 */
} CommutationStep_t;

/* Hall code (1..6) -> logical phase roles. See file header for the P1/P2/P3
 * -> physical channel mapping (fixed for P1, swappable for P2/P3). */
static const CommutationStep_t s_hallTable[8] =
{
  /* 0 */ {{PH_OFF, PH_OFF, PH_OFF}},   /* invalid */
  /* 1 */ {{PH_PWM, PH_OFF, PH_ON }},
  /* 2 */ {{PH_OFF, PH_ON,  PH_PWM}},
  /* 3 */ {{PH_PWM, PH_ON,  PH_OFF}},
  /* 4 */ {{PH_ON,  PH_PWM, PH_OFF}},
  /* 5 */ {{PH_OFF, PH_PWM, PH_ON }},
  /* 6 */ {{PH_ON,  PH_OFF, PH_PWM}},
  /* 7 */ {{PH_OFF, PH_OFF, PH_OFF}},   /* invalid */
};

static volatile BLDC_State_t s_state = BLDC_STOPPED;
static volatile uint8_t      s_hallState = 0;
static volatile uint32_t     s_lastStepTimestamp = 0;
static volatile uint32_t     s_lastStepIntervalUs = 0xFFFFFFFFU;

static inline uint8_t ReadHallCode(void)
{
  uint8_t h1 = (HALL1_GPIO_Port->IDR & HALL1_Pin) ? 1U : 0U;
  uint8_t h2 = (HALL2_GPIO_Port->IDR & HALL2_Pin) ? 1U : 0U;
  uint8_t h3 = (HALL3_GPIO_Port->IDR & HALL3_Pin) ? 1U : 0U;
  return (uint8_t)((h3 << 2) | (h2 << 1) | h1);
}

/* Write OCxM (CCMR1/CCMR2) and CCxE/CCxNE (CCER) for one physical TIM1 channel (1..3).
 * TIM_OCMODE_PWM1 / TIM_OCMODE_FORCED_INACTIVE are pre-positioned for the OC1M
 * field (CCMR1 bits [16,6:4]). OC3M sits at the same bit offset in CCMR2, so
 * channel 3 reuses the value as-is; OC2M sits 8 bits higher in CCMR1 (bits
 * [24,14:12]), so channel 2 needs an extra <<8. */
static void ApplyPhaseRole(uint8_t channel, PhaseRole_t role)
{
  uint32_t ocm = (role == PH_ON) ? TIM_OCMODE_FORCED_INACTIVE : TIM_OCMODE_PWM1;
  uint32_t ccxe_mask, ccxne_mask;

  switch (channel)
  {
    case 1:
      TIM1->CCMR1 = (TIM1->CCMR1 & ~TIM_CCMR1_OC1M) | ocm;
      ccxe_mask  = TIM_CCER_CC1E;
      ccxne_mask = TIM_CCER_CC1NE;
      break;
    case 2:
      TIM1->CCMR1 = (TIM1->CCMR1 & ~TIM_CCMR1_OC2M) | (ocm << 8);
      ccxe_mask  = TIM_CCER_CC2E;
      ccxne_mask = TIM_CCER_CC2NE;
      break;
    case 3:
    default:
      TIM1->CCMR2 = (TIM1->CCMR2 & ~TIM_CCMR2_OC3M) | ocm;
      ccxe_mask  = TIM_CCER_CC3E;
      ccxne_mask = TIM_CCER_CC3NE;
      break;
  }

  if (role == PH_OFF)
  {
    TIM1->CCER &= ~(ccxe_mask | ccxne_mask);
  }
  else
  {
    TIM1->CCER |= (ccxe_mask | ccxne_mask);
  }
}

static void ApplyCommutationStep(uint8_t hallCode)
{
  const CommutationStep_t *step = &s_hallTable[hallCode & 0x7U];

#if (MOTOR_HALL_TABLE_SELECT == MOTOR_HALL_TABLE_REVERSE)
  ApplyPhaseRole(1U, step->role[0]);
  ApplyPhaseRole(2U, step->role[2]);   /* P2/P3 swapped -> reverses rotation direction */
  ApplyPhaseRole(3U, step->role[1]);
#else
  ApplyPhaseRole(1U, step->role[0]);
  ApplyPhaseRole(2U, step->role[1]);
  ApplyPhaseRole(3U, step->role[2]);
#endif
}

void BLDC_Init(void)
{
  s_state = BLDC_STOPPED;
  s_hallState = ReadHallCode();
  s_lastStepTimestamp = 0;
  s_lastStepIntervalUs = 0xFFFFFFFFU;
  TIM1->BDTR &= ~TIM_BDTR_MOE;
  TIM1->CCER &= ~(TIM_CCER_CC1E | TIM_CCER_CC1NE | TIM_CCER_CC2E | TIM_CCER_CC2NE |
                  TIM_CCER_CC3E | TIM_CCER_CC3NE);
}

void BLDC_Start(void)
{
  uint8_t hall = ReadHallCode();

  if (s_state == BLDC_RUNNING)
  {
    return;
  }
  if ((hall == 0U) || (hall == 7U))
  {
    BLDC_EmergencyStop();
    return;
  }

  s_hallState = hall;
  ApplyCommutationStep(hall);
  s_lastStepTimestamp = TIM7->CNT;
  s_lastStepIntervalUs = 0xFFFFFFFFU;
  s_state = BLDC_RUNNING;
  TIM1->BDTR |= TIM_BDTR_MOE;
}

void BLDC_Stop(void)
{
  TIM1->BDTR &= ~TIM_BDTR_MOE;
  TIM1->CCER &= ~(TIM_CCER_CC1E | TIM_CCER_CC1NE | TIM_CCER_CC2E | TIM_CCER_CC2NE |
                  TIM_CCER_CC3E | TIM_CCER_CC3NE);
  s_state = BLDC_STOPPED;
}

void BLDC_EmergencyStop(void)
{
  TIM1->BDTR &= ~TIM_BDTR_MOE;   /* single most important line: kill all outputs now */
  TIM1->CCER &= ~(TIM_CCER_CC1E | TIM_CCER_CC1NE | TIM_CCER_CC2E | TIM_CCER_CC2NE |
                  TIM_CCER_CC3E | TIM_CCER_CC3NE);
  s_state = BLDC_FAULT;
}

void BLDC_ClearFault(void)
{
  if (s_state == BLDC_FAULT)
  {
    s_state = BLDC_STOPPED;
  }
}

void BLDC_SetDutyPermille(uint16_t duty)
{
  uint32_t ccr;

  if (duty > PWM_DUTY_MAX)
  {
    duty = PWM_DUTY_MAX;
  }
  ccr = ((uint32_t)duty * (TIM1_PWM_PERIOD + 1U)) / PWM_DUTY_MAX;
  TIM1->CCR1 = ccr;
  TIM1->CCR2 = ccr;
  TIM1->CCR3 = ccr;
}

BLDC_State_t BLDC_GetState(void)
{
  return s_state;
}

uint8_t BLDC_GetHallState(void)
{
  return s_hallState;
}

uint32_t BLDC_GetStepIntervalUs(void)
{
  return s_lastStepIntervalUs;
}

void BLDC_HallEXTI_Callback(uint16_t GPIO_Pin)
{
  uint32_t now, prev;
  uint8_t hall;

  if ((GPIO_Pin != HALL1_Pin) && (GPIO_Pin != HALL2_Pin) && (GPIO_Pin != HALL3_Pin))
  {
    return;
  }

  hall = ReadHallCode();
  now  = TIM7->CNT;
  prev = s_lastStepTimestamp;
  s_lastStepIntervalUs = (uint32_t)((uint16_t)now - (uint16_t)prev); /* 16-bit wraparound-safe */
  s_lastStepTimestamp  = now;
  s_hallState = hall;

  if (s_state != BLDC_RUNNING)
  {
    return;
  }

  if ((hall == 0U) || (hall == 7U))
  {
    BLDC_EmergencyStop();
    return;
  }

  ApplyCommutationStep(hall);
}
