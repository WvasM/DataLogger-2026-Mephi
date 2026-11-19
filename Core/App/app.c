#include "app/button.h"
#include "app/geiger.h"
#include "app/rtc.h"
#include "ds18b20.h" //драйвер датчика температуры
#include "main.h"
#include "lcd.h"   // Драйвер ST7735
#include "ugui.h"
#include <stdio.h>

float current_temperature = 0.0f; //глобальная переменная для датчика температуры

/* ---------------- Меню ----------------
 * Экраны листаются поворотом энкодера. Добавить экран: новый пункт в enum,
 * название в screen_titles и case в draw_screen(). */

/* Сколько отсчётов TIM3 на один щелчок энкодера. Если экран переключается
 * через щелчок или по два раза за щелчок — поменять на 2 или 4. */
#define ENC_COUNTS_PER_STEP 4

typedef enum {
    SCREEN_RADIATION = 0,
    SCREEN_TEMPERATURE,
    SCREEN_SYSTEM,
    SCREEN_COUNT
} screen_t;

static const char *screen_titles[SCREEN_COUNT] = {
    "Radiation",
    "Temperature",
    "System",
};

/* Область внутри рамки, которую рисует main.c: UG_DrawFrame(5, 30, 155, 120) */
#define AREA_X1 6
#define AREA_Y1 31
#define AREA_X2 154
#define AREA_Y2 119

/* Данные, которые показывают экраны */
static uint32_t g_cps  = 0;
static float    g_cpm  = 0;
static float    g_usvh = 0;

/* Шаги энкодера с прошлого вызова: +1 / -1 за щелчок */
static int8_t encoder_steps(void) {
    static uint16_t last = 0;
    static int16_t acc = 0;
    static uint8_t initialized = 0;

    uint16_t now = (uint16_t)rawCounter;   // TIM3 16-битный, разность через int16 переживает переполнение
    if (!initialized) { last = now; initialized = 1; return 0; }

    acc += (int16_t)(uint16_t)(now - last);
    last = now;

    int8_t steps = 0;
    while (acc >= ENC_COUNTS_PER_STEP)  { acc -= ENC_COUNTS_PER_STEP; steps++; }
    while (acc <= -ENC_COUNTS_PER_STEP) { acc += ENC_COUNTS_PER_STEP; steps--; }
    return steps;
}

/* printf с nano.specs не умеет %f, поэтому дробные числа печатаем через целые */
static void format_temperature(char *buf, size_t size, float t) {
    if (t <= DS18B20_ERROR_TEMP) {
        snprintf(buf, size, " --.-");
        return;
    }
    int32_t t10 = (int32_t)(t * 10.0f + (t >= 0 ? 0.5f : -0.5f));
    char sign = (t10 < 0) ? '-' : ' ';
    if (t10 < 0) t10 = -t10;
    snprintf(buf, size, "%c%3ld.%ld", sign, (long)(t10 / 10), (long)(t10 % 10));
}

static void draw_header(screen_t s) {
    char buf[24];
    snprintf(buf, sizeof buf, "%-12s%u/%u", screen_titles[s], (unsigned)(s + 1), (unsigned)SCREEN_COUNT);
    LCD_PutStr(10, 35, buf, FONT_8X12, C_YELLOW, C_BLACK);
}

/* Строки фиксированной ширины, чтобы новое значение затирало старое */
static void draw_screen(screen_t s) {
    char buf[24];

    switch (s) {
    case SCREEN_RADIATION: {
        snprintf(buf, sizeof buf, "%6lu", (unsigned long)g_cpm);
        LCD_PutStr(10, 52, buf, FONT_16X26, C_WHITE, C_BLACK);
        LCD_PutStr(10, 82, "CPM", FONT_8X12, C_WHITE, C_BLACK);

        uint32_t u100 = (uint32_t)(g_usvh * 100.0f + 0.5f);
        snprintf(buf, sizeof buf, "uSv/h:%3lu.%02lu",
                 (unsigned long)(u100 / 100), (unsigned long)(u100 % 100));
        LCD_PutStr(10, 100, buf, FONT_8X12, C_WHITE, C_BLACK);
        break;
    }
    case SCREEN_TEMPERATURE:
        format_temperature(buf, sizeof buf, current_temperature);
        LCD_PutStr(10, 52, buf, FONT_16X26, C_WHITE, C_BLACK);
        LCD_PutStr(10, 82, "deg C", FONT_8X12, C_WHITE, C_BLACK);
        LCD_PutStr(10, 100,
                   (current_temperature <= DS18B20_ERROR_TEMP) ? "no sensor   " : "DS18B20 PA1 ",
                   FONT_8X12, C_WHITE, C_BLACK);
        break;

    case SCREEN_SYSTEM: {
        char time_buf[TIME_FMT_BUF_SIZE];
        Time_Format(Time_GetElapsedMs(), time_buf);
        time_buf[8] = '\0';                       // "hh:mm:ss" без миллисекунд
        LCD_PutStr(10, 52, time_buf, FONT_16X26, C_WHITE, C_BLACK);
        LCD_PutStr(10, 82, "uptime", FONT_8X12, C_WHITE, C_BLACK);
        snprintf(buf, sizeof buf, "enc:%5lu", (unsigned long)rawCounter);
        LCD_PutStr(10, 100, buf, FONT_8X12, C_WHITE, C_BLACK);
        break;
    }
    default:
        break;
    }
}

static void show_screen(screen_t s) {
    UG_FillFrame(AREA_X1, AREA_Y1, AREA_X2, AREA_Y2, C_BLACK);   // очистить старый экран
    draw_header(s);
    draw_screen(s);
}

void super_loop(void) {
    uint32_t last = 0;
    uint32_t last_1s = 0;
    screen_t screen = SCREEN_RADIATION;

    Time_Init();
    Geiger_Init();
    show_screen(screen);

    while (1) {
        Time_Update();

        rawCounterProcess();

        DS18B20_Process();                             // неблокирующий опрос датчика
        current_temperature = DS18B20_GetTemperature();

        if (HAL_GetTick() - last_1s >= 1000) {
            last_1s = HAL_GetTick();
            g_cps  = Geiger_GetCPS();                  // забирает и обнуляет счётчик
            g_cpm  = Geiger_GetCPM();
            g_usvh = Geiger_GetUSVH();
            (void)g_cps;
        }

        /* Переключение экранов энкодером (по кругу в обе стороны) */
        int8_t steps = encoder_steps();
        if (steps != 0) {
            int32_t next = ((int32_t)screen + steps) % SCREEN_COUNT;
            if (next < 0) next += SCREEN_COUNT;
            screen = (screen_t)next;
            show_screen(screen);
            last = HAL_GetTick();
        }

        /* Обновление значений текущего экрана */
        if (HAL_GetTick() - last >= 100) {
            last = HAL_GetTick();
            draw_screen(screen);
        }
    }
}
