#ifndef __SPEED_PID_H
#define __SPEED_PID_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

void     SpeedPID_Init(void);
void     SpeedPID_Reset(void);          /* clear integrator, call whenever the motor (re)starts */
void     SpeedPID_SetTargetRPM(uint32_t rpm);
uint32_t SpeedPID_GetTargetRPM(void);
uint32_t SpeedPID_GetMeasuredRPM(void);

/* Call every SPEED_CONTROL_PERIOD_MS while BLDC_GetState() == BLDC_RUNNING.
 * Reads the commutation interval from bldc.c, runs the PID and applies the
 * new duty cycle via BLDC_SetDutyPermille(). */
void SpeedPID_Update(void);

#ifdef __cplusplus
}
#endif

#endif /* __SPEED_PID_H */
