#include "app/button.h"
#include "ds18b20.h" //драйвер датчика температуры
#include "main.h"
#include "lcd.h"   // Драйвер ST7735
#include "ugui.h"
#include <stdio.h>
float current_temperature = 0.0f; //глобальная переменная для датчика температуры
#include "Inc/app/geiger.h"
#include "Inc/app/rtc.h"

void super_loop(void) {
    char buf[24];
    char time_buf[16];
    uint32_t last = 0;
    uint32_t last_1s = 0;

    uint32_t cps = 0;
    float cpm = 0;
    float usvh = 0;
    Geiger_Init();

    while (1) {
        Time_Update();

        rawCounterProcess();

        current_temperature = DS18B20_ReadTemperature(); //опрос датчика температуры
        if(HAL_GetTick() - last_1s >= 1000){
            cps = Geiger_GetCPS();
            cpm = Geiger_GetCPM();
            usvh = Geiger_GetUSVH();
        }

        if (HAL_GetTick() - last >= 100) {
            last = HAL_GetTick();
            snprintf(buf, sizeof buf, "enc:%5lu", (unsigned long)rawCounter);
            LCD_PutStr(10, 50, buf, FONT_8X12, C_WHITE, C_BLACK);

            uint32_t ms = Time_GetElapsedMs();
            Time_Format(uint32_t ms, char *time_buf);
        }
    }
}
