/*********
 * Thsi file initializes pins on the esp32-S2_Solo-2
 * 
 * 
 */

#include "driver/i2c.h"
#include "pin_config.h"
#include "esp_err.h"
#include "esp_log.h" 

static const char *TAG = "I2C";

//Setup for the I2C interface for the esp32 controller
void i2c_master_config()
{
    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_PIN,
        .sda_pullup_en = GPIO_PULLUP_DISABLE,
        .scl_io_num = I2C_SCL_PIN,
        .scl_pullup_en = GPIO_PULLUP_DISABLE,
        .master.clk_speed = I2C_MASTER_FREQ,
    };

    //not sure if this is needed or how to use this yet
    esp_err_t err = i2c_param_config(I2C_PORT, &i2c_conf);
    if(err == ESP_OK)
    {
        ESP_LOGI(TAG, "Port successfully setup.");

        err = i2c_driver_install(I2C_PORT, I2C_MODE_MASTER, 0, 0, 0);
        if(err == ESP_OK)
        {
            ESP_LOGI(TAG, "Driver installed");
        }
        else
        {
            ESP_LOGE(TAG, "Driver not installed successfully");
        }
    }
    else
    {
        ESP_LOGE(TAG, "Port was not setup succesfully.");
    }
}