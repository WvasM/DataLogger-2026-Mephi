#ifndef GEIGER_H
#define GEIGER_H

#include "Inc/main.h"

void Geiger_Init(void);
uint32_t Geiger_GetCPS(void);
float Geiger_GetCPM(void);
float Geiger_GetUSVH(void);

#endif