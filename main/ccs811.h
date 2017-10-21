#ifndef CCS811_H
#define CCS811_H
#include "esp_system.h"
#include "esp_log.h"
#include "i2c.h"
void initCCS811(void);
void getCCS811(uint16_t *co2,uint16_t *voc);
#endif