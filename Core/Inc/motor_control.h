#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include <stdint.h>

#define MOTOR_HALL_TIMEOUT_MS 500U

void MotorControl_Init(void);
void MotorControl_Start(void);
void MotorControl_Stop(void);
void MotorControl_Toggle(void);
void MotorControl_Process(void);
void MotorControl_OnHallTransition(uint8_t hall, uint8_t valid,
                                   uint8_t transition_valid);
uint8_t MotorControl_IsRunning(void);

#endif
