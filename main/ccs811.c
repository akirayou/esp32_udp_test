#include "ccs811.h"
static const char *TAG = "CCS811";
static const uint8_t ccs811addr=0x5a;

void initCCS811(void)
{
    uint8_t buf[2];
    esp_err_t ret;
    buf[0]=0x20;//register address
    if(ESP_OK != (ret=i2c_write(ccs811addr,buf,1))){ESP_LOGE(TAG,"write Failed on init:%d",ret);}
    if(ESP_OK != (ret=i2c_read(ccs811addr,buf,1))){ESP_LOGE(TAG,"read failed on init:%d",ret);}
    if(0x81!=buf[0]){
        ESP_LOGE(TAG, "... faild to detect CCS811 %d",(int)buf);
    }else{
        ESP_LOGI(TAG, "... CCS811 found") ;   
    }

    //start CCS811
    buf[0]=0xf4;
    if(ESP_OK != (ret=i2c_write(ccs811addr,buf,1))){ESP_LOGE(TAG,"write Failed on app_start:%d",ret);}
    buf[0]=0x01;
    buf[1]=0x10;
    if(ESP_OK != (ret=i2c_write(ccs811addr,buf,2))){ESP_LOGE(TAG,"write Failed on set measure mode:%d",ret);}
}

void getCCS811(uint16_t *co2,uint16_t *voc)
{
    uint8_t buf[8];
    esp_err_t ret;
    buf[0]=0x02;
    if(ESP_OK != (ret=i2c_write(ccs811addr,buf,1))){ESP_LOGE(TAG,"write address Failed on read:%d",ret);}
    if(ESP_OK != (ret=i2c_read(ccs811addr,buf,8))){ESP_LOGE(TAG,"read failed on init:%d",ret);}
    *co2=(uint16_t)buf[0]*256+buf[1];
    *voc=(uint16_t)buf[2]*256+buf[3];
}