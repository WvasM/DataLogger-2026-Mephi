#include "main.h"
#include "app/geiger.h"

#define CPM_PER_USVH 123.147f
#define TUBE_BACKGROUND_CPM 12.0f
#define GEIGER_CONVERSION_FACTOR 0.00812037f

volatile uint32_t pulse_count = 0;

void Geiger_Init(void){
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_SYSCFG_CLK_ENABLE();

    GPIO_InitStruct.Pin = GEIGER_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
    
    pulse_count = 0;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
    if(GPIO_Pin == GEIGER_PIN){
        pulse_count++;
    }
}

uint32_t Geiger_GetCPS(){
    uint32_t cps;
    HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);
    cps = pulse_count;
    pulse_count = 0;
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
    return cps;
}

float Geiger_GetCPM(){
    float cpm;
    cpm = (float)Geiger_GetCPS()*60.0;
    return cpm;
}

float Geiger_GetUSVH(){
    float usvh;
    float cpm = Geiger_GetCPM();
    if(cpm > TUBE_BACKGROUND_CPM){
        usvh = (cpm - TUBE_BACKGROUND_CPM) / CPM_PER_USVH;
    } else {
        usvh = 0.0;
    }
    return usvh;
}