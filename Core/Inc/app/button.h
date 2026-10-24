#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>

extern volatile uint32_t rawCounter;

void button_init(void);
void button_process(void);
void rawCounterProcess(void);

#endif