#include "adsens.h"

static esp_adc_cal_characteristics_t characteristics;
#define V_REF 1100 
void adsense_setup(void )
{
    adc1_config_width(ADC_WIDTH_12Bit);
    esp_adc_cal_get_characteristics(V_REF, ADC_ATTEN_11db, ADC_WIDTH_12Bit, &characteristics);
    //Init for SVP
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_11db);
    //Init for SVN
    adc1_config_channel_atten(ADC1_CHANNEL_3, ADC_ATTEN_11db);
}
uint32_t getSVP(void)//get ad voltage in mV at SVP pin
{
    return adc1_to_voltage(ADC1_CHANNEL_0, &characteristics);
}
uint32_t getSVN(void)// at SVN
{
    return adc1_to_voltage(ADC1_CHANNEL_3, &characteristics);    
}