#include "i2c_config.h"
#include "general_sensors.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "stdint.h"
#include "stdbool.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#define CO2_SENS_ADDR_A    0x62     //0x29
#define CO2_SENS_ADDR_B    0x2A

const char *TAG = "CO2";
SemaphoreHandle_t co2_mutex = NULL;

// Command used before deep sleep to power sensor off and save more power, second command used to wakeup every time device itself wakes
uint8_t power_down_co2_cmd[2] = {0x36, 0xe0};
uint8_t wakeup_co2_cmd[2]     = {0x36, 0xf6};


/******************************
 * @brief This function is responsible for sending a read 
 * @param raw_co2_concentration this is used as an output parameter. This value will be returned so that we can proceed with data
 **************************/
esp_err_t co2_read_data(uint16_t *co2_concentration)
{
    esp_err_t err = ESP_FAIL;
    uint8_t read_cmd[2] =  {0xec, 0x05};   
    uint8_t co2_stop_cmd[2] = {0x3f, 0x86};
    uint8_t sensor_data[3] = {0};

        // Write command to sensor to receive the measured data
        err = i2c_master_transmit_receive(i2c_co2_device_handle, read_cmd, sizeof(read_cmd), sensor_data, sizeof(sensor_data), pdMS_TO_TICKS(100));
        if(err == ESP_OK) 
        {
            *co2_concentration = (sensor_data[0] << 8) | sensor_data[1];
        }
        else
        {
            ESP_LOGE(TAG, "Failed to send write command to sensor at address 0x%02X", i2c_co2_device.device_address);
            return err;
        }

        // Perform CRC, only proceed if check is successful
         
        if (crc_check(sensor_data, 2) != sensor_data[2]) 
        {
            ESP_LOGE(TAG, "CRC mismatch");
            return ESP_FAIL;
        }
        else   // if the crc check was succesful, add the newest read value to the data array
        {
            if(sensor_data_buffer.co2_reading_index < MAX_SENSOR_READINGS)
            {
                sensor_data_buffer.co2_concentration[sensor_data_buffer.co2_reading_index] = *co2_concentration;
                sensor_data_buffer.co2_reading_index++;
            }
        } 

        // have sensor stop taking measurements after a read to save power
        err = i2c_master_transmit(i2c_co2_device_handle, co2_stop_cmd, sizeof(co2_stop_cmd), pdMS_TO_TICKS(100));
        if(err != ESP_OK)
        {
            ESP_LOGE(TAG, "measurement not stopped");
        }

    return ESP_OK;
}


/**********************************
 * @brief This task handles communication between the MCU and the CO2 sensor.
 *        It also handles all logic that needs to be performed on the data read
 **********************************/
void co2_task(void *parameter)
{
    // 32 start CO2 sensor command split byte by byte so that it can be used in the transmit function
    uint8_t co2_start_cmd[2] =    {0x21, 0xb1};                          //{0x36, 0x15, 0x00, 0x11};  //the two commands split into byte by byte  
    
    // Create co2 mutex to ensure reads and writes to co2 data array are not causing race conditions
    co2_mutex = xSemaphoreCreateMutex();
    if(co2_mutex == NULL)
    {
        ESP_LOGE(TAG, "Error creating co2 mutex");
    }

    while(1) // continuous task loop
    {
        esp_err_t err = ESP_FAIL;
        uint16_t co2_concentration = 0;

        if(xSemaphoreTake(co2_mutex, pdMS_TO_TICKS(100)) == pdTRUE) // Ensure that nothing else interacts with the CO2 data while taking a measurement
        {
            // Give other sensor tasks the chance to take their mutex as well, ensures it only goes into deep sleep when needed, and for sensor to wakeup
            vTaskDelay(pdMS_TO_TICKS(100));

            // Wakeup CO2 sensor every time the device itsel awakens, this sensor does not respond to this command, but it is necessary
            i2c_master_transmit(i2c_co2_device_handle, wakeup_co2_cmd, sizeof(wakeup_co2_cmd), pdMS_TO_TICKS(100));

            err = i2c_master_transmit(i2c_co2_device_handle, co2_start_cmd, sizeof(co2_start_cmd), pdMS_TO_TICKS(100));
            if(err != ESP_OK)
            {
                ESP_LOGE(TAG, "Error writing measure command to sensor with error: 0x%03x", err);
            }

            // after sensors receive initial command, give time to take measurement
            vTaskDelay(pdMS_TO_TICKS(5000));
                    
            if(co2_read_data(&co2_concentration) == ESP_OK)
            {
                ESP_LOGI("CO2 Reading", "PPM: %d", co2_concentration);
            }
            
            // Release the semaphore, power down sensor, and delay so that the deep sleep task has time to check that the semaphore was released
            vTaskDelay(pdMS_TO_TICKS(50));
            err = i2c_master_transmit(i2c_co2_device_handle, power_down_co2_cmd, sizeof(power_down_co2_cmd), pdMS_TO_TICKS(100));
            if(err != ESP_OK)
            {
                ESP_LOGE(TAG, "Error powering down CO2 Sensor before Deep Sleep");
            }
            else
            {
                ESP_LOGI(TAG, "Powered down CO2 Sensor");
            }
        }
        xSemaphoreGive(co2_mutex);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
        
}