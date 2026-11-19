#include "app/button.h"
#include "app/geiger.h"
#include "app/rtc.h"
#include "ds18b20.h" //драйвер датчика температуры
#include "main.h"
#include "lcd.h"   // Драйвер ST7735
#include "ugui.h"
#include <stdio.h>

float current_temperature = 0.0f; //глобальная переменная для датчика температуры

/* printf с nano.specs не умеет %f, поэтому дробные числа печатаем через целые */
static void format_temperature(char *buf, size_t size, float t) {
    if (t <= DS18B20_ERROR_TEMP) {
        snprintf(buf, size, "T:   --.- C ");
        return;
    }
    int32_t t10 = (int32_t)(t * 10.0f + (t >= 0 ? 0.5f : -0.5f));
    char sign = (t10 < 0) ? '-' : ' ';
    if (t10 < 0) t10 = -t10;
    snprintf(buf, size, "T: %c%3ld.%ld C ", sign, (long)(t10 / 10), (long)(t10 % 10));
}

void super_loop(void) {
    char buf[24];
    char time_buf[TIME_FMT_BUF_SIZE];
    uint32_t last = 0;
    uint32_t last_1s = 0;

    uint32_t cps = 0;
    float cpm = 0;
    float usvh = 0;

    Time_Init();
    Geiger_Init();

    while (1) {
        Time_Update();

        rawCounterProcess();

        DS18B20_Process();                             // неблокирующий опрос датчика
        current_temperature = DS18B20_GetTemperature();

        if (HAL_GetTick() - last_1s >= 1000) {
            last_1s = HAL_GetTick();
            cps = Geiger_GetCPS();                     // забирает и обнуляет счётчик
            cpm = Geiger_GetCPM();
            usvh = Geiger_GetUSVH();
            (void)cps;

            format_temperature(buf, sizeof buf, current_temperature);
            LCD_PutStr(10, 64, buf, FONT_8X12, C_WHITE, C_BLACK);

            snprintf(buf, sizeof buf, "CPM:  %6lu", (unsigned long)cpm);
            LCD_PutStr(10, 78, buf, FONT_8X12, C_WHITE, C_BLACK);

            uint32_t u100 = (uint32_t)(usvh * 100.0f + 0.5f);
            snprintf(buf, sizeof buf, "uSv/h:%3lu.%02lu",
                     (unsigned long)(u100 / 100), (unsigned long)(u100 % 100));
            LCD_PutStr(10, 92, buf, FONT_8X12, C_WHITE, C_BLACK);
        }

        if (HAL_GetTick() - last >= 100) {
            last = HAL_GetTick();
            snprintf(buf, sizeof buf, "enc:%5lu", (unsigned long)rawCounter);
            LCD_PutStr(10, 50, buf, FONT_8X12, C_WHITE, C_BLACK);

            Time_Format(Time_GetElapsedMs(), time_buf);
            snprintf(buf, sizeof buf, "t:%s", time_buf);
            LCD_PutStr(10, 106, buf, FONT_8X12, C_WHITE, C_BLACK);
        }
    }
}
