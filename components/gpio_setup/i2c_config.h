
#ifndef I2C_CONFIG_H
#define I2C_CONFIG_H

#define I2C_SDA_PIN 33    //26
#define I2C_SCL_PIN 34    //25
#define I2C_MASTER_FREQ 100000
#define I2C_PORT I2C_NUM_0

#include "driver/i2c_master.h"

void i2c_master_config(void);

extern i2c_master_bus_handle_t i2c_bus_handle;

extern i2c_master_dev_handle_t i2c_co2_device_handle;
extern i2c_device_config_t     i2c_co2_device;

extern i2c_master_dev_handle_t i2c_temp_device_handle;
extern i2c_device_config_t     i2c_temp_device;

extern i2c_master_dev_handle_t i2c_voc_device_handle;
extern i2c_device_config_t i2c_voc_device;

extern i2c_master_dev_handle_t i2c_display_device_handle;
extern i2c_device_config_t     i2c_display_device;

#endif