#ifndef I2C_H
#define I2C_H
#include "esp_system.h"
#include "esp_log.h"
#include "driver/i2c.h"
void i2c_setup_master(void);
esp_err_t i2c_write(uint8_t addr, uint8_t* data_wr, size_t size);
esp_err_t i2c_read(uint8_t addr, uint8_t* data_rd, size_t size);
#endif