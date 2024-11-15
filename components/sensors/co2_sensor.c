#include "pin_config.h"
#include "sensor_tasks.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "stdint.h"
#include "stdbool.h"

/**
 * 
 * @param sensor_address this paramter is the sensor address, there are two co2 sensors so this function will be called for each one
 * @param raw_co2_concentration this is used as an output parameter. This value will be returned so that we can proceed with data
 */
esp_err_t co2_read_data(uint8_t sensor_address, uint16_t *raw_co2_concentration)
{
    esp_err_t err = ESP_FAIL;
    uint8_t read_cmd[2] = {0x16, 0x39};
    uint8_t sensor_data[9] = {0};

    err = i2c_master_write_to_device(I2C_PORT, sensor_address, read_cmd, sizeof(read_cmd), I2C_TIMEOUT);
    if(err != ESP_OK) 
    {
        ESP_LOGE("CO2", "Failed to send read command to sensor at address 0x%02X", sensor_address);
        return err;
    }

    err = i2c_master_read_from_device(I2C_PORT, sensor_address, sensor_data, sizeof(sensor_data), I2C_TIMEOUT);
    if(err != ESP_OK) 
    {
        ESP_LOGE("CO2", "Failed to send read command to sensor at address 0x%02X", sensor_address);
        return err;
    }

    //perform crc check
    err = crc_check(sensor_data, 2, sensor_data[2]);
    if(err != ESP_OK)
    {
        ESP_LOGE("CO2", "CRC check failed for sensor at address 0x%02X", sensor_address);
        return  err;
    }

    *raw_co2_concentration = (sensor_data[0] << 8) | sensor_data[1];

    return ESP_OK;
}

/**
 * 
 * @param co2_a co2 percentage measured by co2 sensor A
 * @param co2_b co2 percentage measured by co2 sensor B
 */
bool did_both_co2_sensors_read_valid(float co2_a, float co2_b)
{
    float difference = co2_a - co2_b;

    if( (-0.4 < difference) && (difference < 0.4))
    {
        return true;
    }
    else
    {
        return false;
    }
}