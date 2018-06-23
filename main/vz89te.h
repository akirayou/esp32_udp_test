#ifndef VZ89TE_H
#define VZ89TE_H
#include "esp_system.h"
#include "esp_log.h"
#include "i2c.h"
void initVZ89TE(void);
bool getVZ89TE(uint8_t *co2,uint8_t *voc);
#endif