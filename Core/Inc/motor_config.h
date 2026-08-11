/**
  ******************************************************************************
  * @file    motor_config.h
  * @brief   Central tuning/configuration header for the six-step BLDC drive.
  *          Every value that depends on the physical hardware (motor, shunt
  *          resistors, bus divider, gain resistors ...) and therefore cannot
  *          be known in advance is called out explicitly below - re-check and
  *          adjust these before spinning a real motor.
  ******************************************************************************
  */

#ifndef MOTOR_CONFIG_H
#define MOTOR_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* ===================== Clock tree ===================== *
 * HSI16 / PLLM(4) = 4MHz ; *PLLN(85) = 340MHz VCO ; /PLLR(2) = 170MHz SYSCLK
 * AHB = APB1 = APB2 = 170MHz (no prescaling, all within the 170MHz max spec).
 * No external crystal is used (none of the given pins are OSC_IN/OSC_OUT).
 */
#define SYSCLK_FREQ_HZ                  170000000UL
#define TIM1_CLOCK_HZ                   170000000UL   /* APB2 timer clock, PPRE2 = 1 => TIMxCLK = PCLK2 */

/* ===================== PWM / commutation ===================== */
#define PWM_FREQUENCY_HZ                20000UL
/* Center-aligned mode: Fpwm = TIM1CLK / (2 * (ARR+1))  =>  ARR = TIM1CLK/(2*Fpwm) - 1 */
#define TIM1_PWM_PERIOD                 ((uint16_t)(TIM1_CLOCK_HZ / (2UL * PWM_FREQUENCY_HZ) - 1UL))  /* 4249 */

/* Dead time ~600 ns. DTG[7:5]=0xx range: DT = DTG[7:0] * tDTS, tDTS = 1/TIM1CLK (CKD=0).
 * 600ns / (1/170MHz) = 102 (0x66) -> 102 * 5.882ns = 600ns */
#define TIM1_DEADTIME_DTG                102U

/* PWM duty is expressed 0..1000 (0.1% resolution) throughout the application layer. */
#define PWM_DUTY_MAX                    1000U
#define PWM_DUTY_MIN_START               50U   /* minimum duty to reliably start spinning, tune on hardware */

/* Six-step commutation table selection.
 * The physical phase-to-Hall relationship of a hand-wired board is only known
 * after the first bring-up test. If the motor does not turn, buzzes, or spins
 * only in one direction with excessive current, set this to the other value
 * and re-flash - this is the standard bring-up step for any BLDC hall board. */
#define MOTOR_HALL_TABLE_FORWARD         0
#define MOTOR_HALL_TABLE_REVERSE         1
#define MOTOR_HALL_TABLE_SELECT          MOTOR_HALL_TABLE_FORWARD

/* ===================== Motor / speed feedback ===================== */
/* Number of magnet pole PAIRS of the target motor. Only affects RPM scaling
 * of the speed measurement/PID, NOT the commutation logic itself. MUST be
 * set to the real motor's value for correct RPM readout and PID tuning. */
#define MOTOR_POLE_PAIRS                 7U

/* Free-running speed-capture timer (TIM7) tick rate. */
#define SPEED_TIMER_TICK_HZ              1000000UL   /* 1 MHz -> 1 us/tick */

/* ===================== Speed PID (duty output, 0..PWM_DUTY_MAX) ===================== */
#define SPEED_PID_KP                     0.60f
#define SPEED_PID_KI                     0.08f
#define SPEED_PID_KD                     0.0f
#define SPEED_PID_OUT_MIN                0.0f
#define SPEED_PID_OUT_MAX                ((float)PWM_DUTY_MAX)
#define SPEED_CONTROL_PERIOD_MS           5U   /* 200 Hz speed loop */

#define DEFAULT_TARGET_RPM               0U
#define MAX_TARGET_RPM                   8000U

/* ===================== Protection thresholds ===================== *
 * VBUS: PA0 -> ADC1_IN1 through an external resistor divider whose ratio is
 * BOARD-SPECIFIC. VBUS_DIVIDER_RATIO below assumes a 10k:1k divider
 * (ratio 11.0) as a placeholder - measure the real divider and correct it.
 * Vbus = ADC_raw * VREFP_VOLTAGE / 4095 * VBUS_DIVIDER_RATIO
 */
#define VREFP_VOLTAGE                    3.3f
#define VBUS_DIVIDER_RATIO               11.0f
#define VBUS_UNDERVOLTAGE_V               8.0f
#define VBUS_OVERVOLTAGE_V               28.0f

/* Phase current: OPAMP1/2/3 are used in Standalone mode (external gain-setting
 * resistors on the shunt amplifier network, values unknown here) with the
 * amplified output routed internally to ADC1_IN3 (U), ADC2_IN3 (V) and
 * ADC1_IN12 (W). Because the real shunt/gain values are board-specific, the
 * overcurrent limit is expressed directly in raw ADC counts around the
 * amplifier's mid-scale (bidirectional shunt signal). Calibrate
 * CURRENT_ADC_ZERO (reading at 0A, motor stopped) and CURRENT_ADC_TRIP
 * (counts away from zero that correspond to your real trip current) on the
 * bench before relying on this for protection. */
#define CURRENT_ADC_ZERO                 2048U
#define CURRENT_ADC_TRIP_DELTA            1200U

/* ===================== UART2 (PB3=TX, PB4=RX) ===================== */
#define UART_BAUDRATE                    115200UL
#define UART_RX_LINE_MAX                 64U
#define TELEMETRY_PERIOD_MS               200U

/* ===================== Button (PC10) ===================== */
#define BUTTON_DEBOUNCE_MS                30U
#define BUTTON_LONGPRESS_MS               800U

#ifdef __cplusplus
}
#endif

#endif /* MOTOR_CONFIG_H */
