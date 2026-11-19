#include "main.h"
#include "app/geiger.h"

#define CPM_PER_USVH 123.147f
#define TUBE_BACKGROUND_CPM 12.0f
#define GEIGER_CONVERSION_FACTOR 0.00812037f

static volatile uint32_t pulse_count = 0;
static uint32_t last_cps = 0;   /* импульсы за последнюю секунду */

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
    return last_cps;
}

float Geiger_GetCPM(void){
    return (float)last_cps * 60.0f;
}

float Geiger_GetUSVH(void){
    float cpm = Geiger_GetCPM();
    if(cpm > TUBE_BACKGROUND_CPM){
        return (cpm - TUBE_BACKGROUND_CPM) / CPM_PER_USVH;
    }
    return 0.0f;
}
