#include "i2c_config.h"
#include "sensor_tasks.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "stdint.h"
#include "stdbool.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define CRC8_POLYNOMIAL 0x31
#define CRC8_INIT 0xff

float co2_value;

/***************
 * @brief CRC function for breadboard CO2 sensor
 * @param data the data read from the sensor
 * @param count amount of bytes
 ***************/
uint8_t sensirion_common_generate_crc(const uint8_t* data, uint16_t count) 
{
    uint16_t current_byte;
    uint8_t crc = CRC8_INIT;
    uint8_t crc_bit;
    /* calculates 8-Bit checksum with given polynomial */
    for (current_byte = 0; current_byte < count; ++current_byte) 
    {
        crc ^= (data[current_byte]);
        for (crc_bit = 8; crc_bit > 0; --crc_bit) 
        {
            if (crc & 0x80)
                crc = (crc << 1) ^ CRC8_POLYNOMIAL;
            else
                crc = (crc << 1);
        }
    }
    return crc;
}




/*****************
 * @brief This function simply takes the read data from the sensor, and performs the equation taken from the datasheet
 *        in order to convert this raw value into a PPM value
 * @param raw_co2_concentration is the raw value read from the CO2 sensor
 ****************/
void convert_co2_data_to_readable(uint16_t *raw_co2_concentration)
{
    co2_value = 0;
    co2_value = ((float)((*raw_co2_concentration - 16384)) / 32768) * 100;

    if(co2_value < 0)
    {
        ESP_LOGE("CO2 Reading", "Value invalid.");
    }
    else
    {
        ESP_LOGI("CO2 Value", "%f", co2_value);
    }
}




/**
 * @brief This function is responsible for sending a read 
 * @param raw_co2_concentration this is used as an output parameter. This value will be returned so that we can proceed with data
 */
esp_err_t co2_read_data(uint16_t *raw_co2_concentration)
{
    esp_err_t err = ESP_FAIL;
    uint8_t read_cmd[2] =  {0xec, 0x05};        //{0x16, 0x39};  This is the command for our PCB sensor
    uint8_t co2_stop_cmd[2] = {0x3f, 0x86};
    uint8_t sensor_data[3] = {0};

    if(xSemaphoreTake(co2_mutex, pdMS_TO_TICKS(100)) == pdTRUE) // Ensure that nothing else interacts with the CO2 data while taking a measurement
    {
        // Write command to sensor to receive the measured data
        err = i2c_master_transmit_receive(i2c_co2_device_handle, read_cmd, sizeof(read_cmd), sensor_data, sizeof(sensor_data), pdMS_TO_TICKS(100));
        if(err == ESP_OK) 
        {
            *raw_co2_concentration = (sensor_data[0] << 8) | sensor_data[1];
        }
        else
        {
            ESP_LOGE("CO2", "Failed to send write command to sensor at address 0x%02X", i2c_co2_device.device_address);
            return err;
        }

        // Perform CRC, only proceed if check is successful
        uint16_t calculated_crc = sensirion_common_generate_crc(sensor_data, 2);
        if (calculated_crc != sensor_data[2]) 
        {
            ESP_LOGE("CO2", "CRC mismatch! Received: 0x%02X, Calculated: 0x%02X", sensor_data[2], calculated_crc);
            return ESP_FAIL;
        }
        else   // if the crc check was succesful and data is valid, add the measured CO2 value to a queue which will be used by the web server
        {
            if(xQueueSend(co2_data_queue, raw_co2_concentration, pdMS_TO_TICKS(5)) != pdTRUE)
            {
                ESP_LOGI("CO2", "Error adding item to queue");
            }
        } 

        // have sensor stop taking measurements after a read to save power
        err = i2c_master_transmit(i2c_co2_device_handle, co2_stop_cmd, sizeof(co2_stop_cmd), pdMS_TO_TICKS(100));
        if(err != ESP_OK)
        {
            ESP_LOGE("CO2", "measurement not stopped");
        }
    }

    // release mutex and delay
    xSemaphoreGive(co2_mutex);
    vTaskDelay(pdMS_TO_TICKS(500));

    return ESP_OK;
}

/**
 * @brief Ensure both sensors read a valid value. Further work needed if one did not
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