#ifndef RTT_LOG_H
#define RTT_LOG_H

#include <stdint.h>

void RTT_Log_Init(void);
void RTT_Log_String(const char *text);
void RTT_Log_Hall(uint8_t hall, uint8_t valid, uint8_t transition_valid,
                  uint32_t edge_count);
void RTT_Log_Status(uint8_t hall, uint32_t rpm, uint32_t edge_count);

#endif
