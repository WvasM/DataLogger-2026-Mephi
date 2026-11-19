#ifndef RTC_H
#define RTC_H

#include <stdint.h>

#define TIME_FMT_BUF_SIZE 16  /* "hh:mm:ss.mmm" + запас */

typedef struct {
    uint32_t h, m, s, ms;
} TimeParts;

void Time_Init(void);
void Time_Update(void);
uint32_t Time_GetUptimeMs(void);
uint32_t Time_GetElapsedMs(void);
void Time_AdjustOffsetMs(int32_t delta_ms);
void Time_ResetOffset(void);
void Time_ToParts(uint32_t ms, TimeParts *out);
void Time_Format(uint32_t ms, char *buf);

#endif