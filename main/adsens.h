#ifndef ADSENSE_H
#define ADSENSE_H
#include "esp_system.h"
#include "esp_log.h"
#include <driver/adc.h>
#include <esp_adc_cal.h>
void adsense_setup(void );
uint32_t getSVP(void);//get ad voltage in mV at SVP pin
uint32_t getSVN(void);// at SVN

#endif