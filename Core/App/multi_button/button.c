#include "app/button.h"
#include "main.h"
#include "lwbtn/lwbtn.h"

static lwbtn_btn_t button;

static uint8_t button_get_state(lwbtn_t *lwbtn, lwbtn_btn_t *btn)
{
    (void)lwbtn; (void)btn;
    return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6) == GPIO_PIN_RESET;
}

static void button_event(lwbtn_t *lwbtn, lwbtn_btn_t *btn, lwbtn_evt_t event)
{
    (void)lwbtn;(void)btn;

    if (event == LWBTN_EVT_ONCLICK) {
        // TODO: Handle button click event
    }
}

void button_init(void)
{
    lwbtn_init(&button, 1, button_get_state, button_event);
}

void button_process(void)
{
    lwbtn_process(HAL_GetTick());
}

volatile uint32_t rawCounter = 0;
uint32_t counter = 0;
volatile uint32_t last_counter = 0;

extern TIM_HandleTypeDef htim3;

void rawCounterProcess(void) {
    rawCounter = __HAL_TIM_GET_COUNTER(&htim3);

    int16_t delta = (int16_t)(rawCounter - last_counter);
    last_counter = rawCounter;

    counter = (rawCounter / 4) % 20;

}