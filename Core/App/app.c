#include "app/button.h"
#include "main.h"
#include "lcd.h"   // Драйвер ST7735
#include "ugui.h"
#include <stdio.h>


void super_loop(void) {
    char buf[24];
    uint32_t last = 0;

    while (1) {
        rawCounterProcess();

        if (HAL_GetTick() - last >= 100) {
            last = HAL_GetTick();
            snprintf(buf, sizeof buf, "enc:%5lu", (unsigned long)rawCounter);
            LCD_PutStr(10, 50, buf, FONT_8X12, C_WHITE, C_BLACK);
        }
    }
}