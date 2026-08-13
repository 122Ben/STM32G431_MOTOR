#include "rtt_log.h"
#include "stm32g4xx_hal.h"
#include <string.h>

#define RTT_BUFFER_SIZE 1024U
#define RTT_ID           "SEGGER RTT"

typedef struct
{
  const char *name;
  char *buffer;
  uint32_t size;
  volatile uint32_t write_offset;
  volatile uint32_t read_offset;
  uint32_t flags;
} RTT_BUFFER_UP;

typedef struct
{
  const char *name;
  char *buffer;
  uint32_t size;
  volatile uint32_t write_offset;
  volatile uint32_t read_offset;
  uint32_t flags;
} RTT_BUFFER_DOWN;

typedef struct
{
  char id[16];
  int32_t max_up_buffers;
  int32_t max_down_buffers;
  RTT_BUFFER_UP up[1];
  RTT_BUFFER_DOWN down[1];
} RTT_CONTROL_BLOCK;

static char rtt_up_buffer[RTT_BUFFER_SIZE];
static char rtt_down_buffer[16];
static RTT_CONTROL_BLOCK rtt_control;

static void RTT_Write(const char *data, uint32_t length)
{
  uint32_t write;
  uint32_t read;
  uint32_t next;

  while (length-- > 0U)
  {
    write = rtt_control.up[0].write_offset;
    read = rtt_control.up[0].read_offset;
    next = write + 1U;
    if (next >= RTT_BUFFER_SIZE)
    {
      next = 0U;
    }
    if (next == read)
    {
      break;
    }
    rtt_up_buffer[write] = *data++;
    __DMB();
    rtt_control.up[0].write_offset = next;
  }
}

void RTT_Log_Init(void)
{
  memset(&rtt_control, 0, sizeof(rtt_control));
  memcpy(rtt_control.id, RTT_ID, sizeof(RTT_ID));
  rtt_control.max_up_buffers = 1;
  rtt_control.max_down_buffers = 1;
  rtt_control.up[0].name = "Terminal";
  rtt_control.up[0].buffer = rtt_up_buffer;
  rtt_control.up[0].size = RTT_BUFFER_SIZE;
  rtt_control.down[0].name = "Terminal";
  rtt_control.down[0].buffer = rtt_down_buffer;
  rtt_control.down[0].size = sizeof(rtt_down_buffer);
  __DMB();
  RTT_Log_String("\r\nRTT hall monitor ready\r\n");
}

void RTT_Log_String(const char *text)
{
  RTT_Write(text, (uint32_t)strlen(text));
}

void RTT_Log_Hall(uint8_t hall, uint8_t valid, uint8_t transition_valid,
                  uint32_t edge_count)
{
  char line[48];
  uint32_t pos = 0U;
  uint32_t divisor = 1000000000U;
  uint8_t started = 0U;

  line[pos++] = 'H'; line[pos++] = '=';
  line[pos++] = (char)('0' + ((hall >> 2) & 1U));
  line[pos++] = (char)('0' + ((hall >> 1) & 1U));
  line[pos++] = (char)('0' + (hall & 1U));
  line[pos++] = ' ';
  if (valid == 0U)
  {
    line[pos++] = 'I'; line[pos++] = 'N'; line[pos++] = 'V';
  }
  else if (transition_valid == 0U)
  {
    line[pos++] = 'J'; line[pos++] = 'U'; line[pos++] = 'M'; line[pos++] = 'P';
  }
  else
  {
    line[pos++] = 'O'; line[pos++] = 'K';
  }
  line[pos++] = ' '; line[pos++] = 'N'; line[pos++] = '=';
  while (divisor > 0U)
  {
    uint8_t digit = (uint8_t)((edge_count / divisor) % 10U);
    if ((digit != 0U) || (started != 0U) || (divisor == 1U))
    {
      line[pos++] = (char)('0' + digit);
      started = 1U;
    }
    divisor /= 10U;
  }
  line[pos++] = '\r'; line[pos++] = '\n';
  RTT_Write(line, pos);
}

static uint32_t RTT_AppendUnsigned(char *line, uint32_t pos, uint32_t value)
{
  char digits[10];
  uint32_t count = 0U;

  do
  {
    digits[count++] = (char)('0' + (value % 10U));
    value /= 10U;
  } while (value != 0U);
  while (count != 0U)
  {
    line[pos++] = digits[--count];
  }
  return pos;
}

void RTT_Log_Status(uint8_t hall, uint32_t rpm, uint32_t edge_count)
{
  char line[48];
  uint32_t pos = 0U;

  memcpy(&line[pos], "RUN H=", 6U); pos += 6U;
  line[pos++] = (char)('0' + ((hall >> 2) & 1U));
  line[pos++] = (char)('0' + ((hall >> 1) & 1U));
  line[pos++] = (char)('0' + (hall & 1U));
  memcpy(&line[pos], " RPM=", 5U); pos += 5U;
  pos = RTT_AppendUnsigned(line, pos, rpm);
  memcpy(&line[pos], " N=", 3U); pos += 3U;
  pos = RTT_AppendUnsigned(line, pos, edge_count);
  line[pos++] = '\r'; line[pos++] = '\n';
  RTT_Write(line, pos);
}
