#include "ds18b20.h"

#define DS18B20_CONV_TIME_MS 750U   /* 12 бит */
#define DS18B20_PERIOD_MS    1000U

static TIM_HandleTypeDef *ds18b20_tim;

static void delay_us(uint16_t us) {
    __HAL_TIM_SET_COUNTER(ds18b20_tim, 0);
    while (__HAL_TIM_GET_COUNTER(ds18b20_tim) < us);
}

static void Set_Pin_Output(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DS18B20_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DS18B20_PORT, &GPIO_InitStruct);
}

static uint8_t DS18B20_Start(void) {
    uint8_t Response = 0;
    Set_Pin_Output();
    HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
    delay_us(480);

    HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_SET);
    delay_us(80);

    if (!(HAL_GPIO_ReadPin(DS18B20_PORT, DS18B20_PIN))) Response = 1;
    else Response = 0;

    delay_us(400);
    return Response;
}

static void DS18B20_Write(uint8_t data) {
    Set_Pin_Output();
    for (int i=0; i<8; i++) {
        if ((data & (1<<i)) != 0) {
            HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
            delay_us(1);
            HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_SET);
            delay_us(60);
        } else {
            HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
            delay_us(60);
            HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_SET);
            delay_us(1);
        }
    }
}

static uint8_t DS18B20_Read(void) {
    uint8_t value = 0;
    for (int i=0; i<8; i++) {
        Set_Pin_Output();
        HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
        delay_us(2);
        HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_SET);
        delay_us(10);
        if (HAL_GPIO_ReadPin(DS18B20_PORT, DS18B20_PIN)) {
            value |= (1<<i);
        }
        delay_us(50);
    }
    return value;
}

/* Запуск преобразования. 1 — датчик ответил */
static uint8_t DS18B20_StartConversion(void) {
    if (!DS18B20_Start()) return 0;
    DS18B20_Write(0xCC);   /* Skip ROM */
    DS18B20_Write(0x44);   /* Convert T */
    return 1;
}

/* Чтение результата (не раньше чем через 750 мс после запуска) */
static float DS18B20_ReadResult(void) {
    if (!DS18B20_Start()) return DS18B20_ERROR_TEMP;
    DS18B20_Write(0xCC);   /* Skip ROM */
    DS18B20_Write(0xBE);   /* Read Scratchpad */

    uint8_t Temp_LSB = DS18B20_Read();
    uint8_t Temp_MSB = DS18B20_Read();

    /* int16_t — чтобы отрицательные температуры считались правильно */
    int16_t raw_temp = (int16_t)(((uint16_t)Temp_MSB << 8) | Temp_LSB);
    return (float)raw_temp * 0.0625f;
}

void DS18B20_Init(TIM_HandleTypeDef *htim) {
    ds18b20_tim = htim;
    HAL_TIM_Base_Start(ds18b20_tim);
}

/* ---- неблокирующий опрос ---- */
static enum { DS_IDLE, DS_CONVERTING } ds_state = DS_IDLE;
static uint32_t ds_t_start = 0;
static float ds_last_temp = DS18B20_ERROR_TEMP;

void DS18B20_Process(void) {
    uint32_t now = HAL_GetTick();

    if (ds_state == DS_IDLE) {
        if (now - ds_t_start >= DS18B20_PERIOD_MS) {
            ds_t_start = now;
            if (DS18B20_StartConversion()) {
                ds_state = DS_CONVERTING;
            } else {
                ds_last_temp = DS18B20_ERROR_TEMP;   /* датчик не ответил */
            }
        }
    } else { /* DS_CONVERTING */
        if (now - ds_t_start >= DS18B20_CONV_TIME_MS) {
            ds_last_temp = DS18B20_ReadResult();
            ds_state = DS_IDLE;
        }
    }
}

float DS18B20_GetTemperature(void) {
    return ds_last_temp;
}

float DS18B20_ReadTemperature(void) {
    if (!DS18B20_StartConversion()) return DS18B20_ERROR_TEMP;
    HAL_Delay(DS18B20_CONV_TIME_MS);
    return DS18B20_ReadResult();
}
