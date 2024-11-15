
#ifndef PIN_CONFIG_H
#define PIN_CONFIG_H

#define I2C_SDA_PIN 45
#define I2C_SCL_PIN 34
#define I2C_MASTER_FREQ 400000
#define I2C_PORT I2C_NUM_0

void i2c_master_config(void);

#endif