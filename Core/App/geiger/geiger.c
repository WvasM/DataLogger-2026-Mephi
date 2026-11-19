#include "main.h"
#include "app/geiger.h"

#define CPM_PER_USVH 123.147f
#define TUBE_BACKGROUND_CPM 12.0f
#define GEIGER_CONVERSION_FACTOR 0.00812037f

#define WINDOW_SECONDS 60

static volatile uint32_t pulse_count = 0;
static uint32_t last_cps = 0;   /* импульсы за последнюю секунду */

/* Скользящее окно: импульсы за каждую из последних 60 секунд.
 * Каждую секунду самая старая ячейка заменяется новой, сумма = CPM. */
static uint16_t window_counts[WINDOW_SECONDS];
static uint8_t  window_pos = 0;      /* куда писать следующую секунду */
static uint8_t  window_filled = 0;   /* сколько секунд уже накоплено (до 60) */
static uint32_t window_sum = 0;      /* сумма всех ячеек */

void Geiger_Init(void){
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_SYSCFG_CLK_ENABLE();

    GPIO_InitStruct.Pin = GEIGER_PIN;
    /* RadiationD v1.1 (CAJOE): VIN в покое высокий, импульс частицы — вниз */
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GEIGER_GPIO_PORT, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

    pulse_count = 0;
    last_cps = 0;
    for (int i = 0; i < WINDOW_SECONDS; i++) window_counts[i] = 0;
    window_pos = 0;
    window_filled = 0;
    window_sum = 0;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
    if(GPIO_Pin == GEIGER_PIN){
        pulse_count++;
    }
}

/* Вызывать ровно раз в секунду: забирает накопленные импульсы и обнуляет
 * счётчик. CPM и мкЗв/ч считаются от этого значения и счётчик НЕ трогают. */
uint32_t Geiger_GetCPS(void){
    HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);
    last_cps = pulse_count;
    pulse_count = 0;
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

    /* сдвигаем окно: выкидываем самую старую секунду, кладём новую */
    uint16_t c = (last_cps > 0xFFFF) ? 0xFFFF : (uint16_t)last_cps;
    window_sum -= window_counts[window_pos];
    window_counts[window_pos] = c;
    window_sum += c;
    window_pos = (uint8_t)((window_pos + 1) % WINDOW_SECONDS);
    if (window_filled < WINDOW_SECONDS) window_filled++;

    return last_cps;
}

/* Импульсы за последние 60 секунд. Пока окно не заполнилось (первая минута
 * после включения), пересчитываем накопленное на минуту — это оценка. */
float Geiger_GetCPM(void){
    if (window_filled == 0) return 0.0f;
    return (float)window_sum * (float)WINDOW_SECONDS / (float)window_filled;
}

float Geiger_GetUSVH(void){
    float cpm = Geiger_GetCPM();
    if(cpm > TUBE_BACKGROUND_CPM){
        return (cpm - TUBE_BACKGROUND_CPM) / CPM_PER_USVH;
    }
    return 0.0f;
}
