#ifndef __UART_PROTOCOL_H
#define __UART_PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/* ASCII line protocol over USART2, 115200-8N1, lines end with '\n' ('\r' ignored):
 *   S            start the motor (closed-loop speed PID, target set by 'R')
 *   X            stop the motor
 *   C            clear a latched fault
 *   R<rpm>       set target RPM for the closed-loop speed PID, e.g. "R3000"
 *   D<permille>  open-loop duty test (0..1000, i.e. 0.1% steps), e.g. "D200" = 20%
 *                (switches to open-loop mode; 'R' switches back to closed-loop)
 *   ?            request one telemetry line immediately
 *
 * Telemetry (sent automatically every TELEMETRY_PERIOD_MS, and on '?'):
 *   T VBUS=<V> IU=<raw> IV=<raw> IW=<raw> RPM=<meas> TARGET=<rpm> DUTY=<permille> STATE=<STOPPED|RUNNING|FAULT> FAULT=<code>
 */

void    UART_Protocol_Init(void);
void    UART_Protocol_Process(void);      /* call every main-loop iteration */
void    UART_Protocol_TelemetryTick(void);/* call every main-loop iteration; internally rate-limited */
uint8_t UART_Protocol_IsOpenLoop(void);

#ifdef __cplusplus
}
#endif

#endif /* __UART_PROTOCOL_H */
