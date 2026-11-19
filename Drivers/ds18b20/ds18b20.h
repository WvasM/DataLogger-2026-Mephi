#ifndef DRIVERS_DS18B20_DS18B20_H_
#define DRIVERS_DS18B20_DS18B20_H_

#include "stm32f4xx_hal.h"

/* Линия 1-Wire. Нужен внешний подтягивающий резистор ~4.7 кОм к 3.3 В */
#define DS18B20_PORT GPIOA
#define DS18B20_PIN  GPIO_PIN_1

/* Значение, которое возвращается, если датчик не ответил */
#define DS18B20_ERROR_TEMP (-999.0f)

/* Таймер должен тикать с частотой 1 МГц (1 тик = 1 мкс), например TIM1
 * с Prescaler = 83 при 84 МГц. Функция сама запускает таймер. */
void DS18B20_Init(TIM_HandleTypeDef *htim);

/* Неблокирующий режим: вызывать DS18B20_Process() в каждом проходе
 * главного цикла. Раз в секунду запускается преобразование, через 750 мс
 * результат забирается. Последнее значение — DS18B20_GetTemperature(). */
void DS18B20_Process(void);
float DS18B20_GetTemperature(void);

/* Блокирующий вариант (ждёт 750 мс внутри). Оставлен для отладки. */
float DS18B20_ReadTemperature(void);

#endif /* DRIVERS_DS18B20_DS18B20_H_ */
