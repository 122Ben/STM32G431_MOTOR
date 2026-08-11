#ifndef __PROTECTION_H
#define __PROTECTION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

typedef enum
{
  FAULT_NONE = 0,
  FAULT_OVERCURRENT_U,
  FAULT_OVERCURRENT_V,
  FAULT_OVERCURRENT_W,
  FAULT_OVERVOLTAGE,
  FAULT_UNDERVOLTAGE,
  FAULT_HALL_INVALID
} FaultCode_t;

void Protection_Init(void);
/* Call periodically (main loop, ~1kHz) after ADC_SampleAll(). Trips
 * BLDC_EmergencyStop() and latches a fault code if any threshold is exceeded. */
void Protection_Check(void);
void Protection_ClearFault(void);
FaultCode_t Protection_GetLastFault(void);
float Protection_GetVbusVoltage(void);

#ifdef __cplusplus
}
#endif

#endif /* __PROTECTION_H */
