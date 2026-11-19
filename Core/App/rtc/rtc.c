#include "app/rtc.h"
#include "main.h"
#include <stdio.h>

static uint32_t s_uptime_ms = 0;
static uint32_t s_last_tick = 0;
static int64_t s_offset_ms = 0;

void Time_Init(){
    s_uptime_ms = 0;
    s_last_tick = HAL_GetTick();
    s_offset_ms = 0;
}

void Time_Update(){
    uint32_t now = HAL_GetTick();
    s_uptime_ms += (now - s_last_tick);
    s_last_tick = now;
}

uint32_t Time_GetUptimeMs(){
    return s_uptime_ms;
}

uint32_t Time_GetElapsedMs(){
    int64_t e = (int64_t)s_uptime_ms - s_offset_ms;
    return (e<0) ? 0 : (uint32_t)e;
}

void Time_AdjustOffsetMs(int32_t delta_ms){
    s_offset_ms -= delta_ms;
    if(s_offset_ms > (int64_t)s_uptime_ms){
        s_offset_ms = (int64_t)s_uptime_ms;
    }
}

void Time_ResetOffset(){
    s_offset_ms = (int64_t)s_uptime_ms;
}

void Time_ToParts(uint32_t ms, TimeParts *out){
    out->ms = ms % 1000;
    uint32_t total_s = ms / 1000;
    out->s = total_s % 60;
    out->m = (total_s / 60) % 60;
    out->h = total_s / 3600;
}

void Time_Format(uint32_t ms, char *buf){
    TimeParts p;
    Time_ToParts(ms,&p);
    snprintf(buf, TIME_FMT_BUF_SIZE, "%02lu:%02lu:%02lu.%03lu",
             (unsigned long)p.h, (unsigned long)p.m, (unsigned long)p.s, (unsigned long)p.ms);
}