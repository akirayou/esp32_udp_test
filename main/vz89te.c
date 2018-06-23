#include "ccs811.h"
static const char *TAG = "VT89TE";
static const uint8_t addr = 0x70;

uint8_t crc8(uint8_t *data, uint8_t size)
{
    uint8_t crc = 0x00;
    uint8_t i = 0x00;
    uint16_t sum = 0x0000;
    for (i = 0; i < size; i++)
    {
        sum = crc + data[i];
        crc = (uint8_t)sum;
        crc += (sum >> 8);
    }
    crc = 0xFF - crc;
    return crc;
}
void initVZ89TE(void)
{
  //nothing 
}

bool getVZ89TE(uint8_t *co2,uint8_t *voc)
{
    const uint8_t readCmd[]={0x0C,0x00,0x00,0x00, 0x00,0xff-0x0C};

    uint8_t buf[7];
    esp_err_t ret;
    buf[0]=0x02;
    if(ESP_OK != (ret=i2c_write(addr,readCmd,7))){
        ESP_LOGE(TAG,"write Failed on read:%d",ret);
        return false;
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
    if(ESP_OK != (ret=i2c_read(addr,buf,7))){
        ESP_LOGE(TAG,"read failed on read:%d",ret);
        return false;
    }
    *co2=(uint16_t)buf[1];
    *voc=(uint16_t)buf[0];
    if(buf[6]!=crc8(buf,6)){
        ESP_LOGE(TAG,"crc error");
        return false;
    }
    return true;
}