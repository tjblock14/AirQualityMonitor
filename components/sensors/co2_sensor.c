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

float co2_value;
QueueHandle_t co2_data_queue = NULL;
SemaphoreHandle_t co2_mutex = NULL;


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

    if(xSemaphoreTake(co2_mutex, pdMS_TO_TICKS(100)) == pdTRUE) // Ensure that nothing else interacts with the CO2 data while taking a measurement
    {
        // Write command to sensor to receive the measured data
        err = i2c_master_transmit_receive(i2c_co2_device_handle, read_cmd, sizeof(read_cmd), sensor_data, sizeof(sensor_data), pdMS_TO_TICKS(100));
        if(err == ESP_OK) 
        {
            *co2_concentration = (sensor_data[0] << 8) | sensor_data[1];
        }
        else
        {
            ESP_LOGE("CO2", "Failed to send write command to sensor at address 0x%02X", i2c_co2_device.device_address);
            return err;
        }

        // Perform CRC, only proceed if check is successful
         
        if (crc_check(sensor_data, 2) != sensor_data[2]) 
        {
            ESP_LOGE("CO2", "CRC mismatch");
            return ESP_FAIL;
        }
        else   // if the crc check was succesful and data is valid, add the measured CO2 value to a queue which will be used by the web server
        {
            if(xQueueSend(co2_data_queue, co2_concentration, pdMS_TO_TICKS(5)) != pdTRUE)
            {
                ESP_LOGE("CO2", "Error adding item to queue");
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


/**********************************
 * @brief This task handles communication between the MCU and the CO2 sensor.
 *        It also handles all logic that needs to be performed on the data read
 **********************************/
void co2_task(void *parameter)
{
    // 32 start CO2 sensor command split byte by byte so that it can be used in the transmit function
    uint8_t co2_start_cmd[2] =    {0x21, 0xb1};                          //{0x36, 0x15, 0x00, 0x11};  //the two commands split into byte by byte  
    
    // create co2 queue and mutex for data transfer with web UI task
    co2_data_queue = xQueueCreate(10, sizeof(uint16_t));
    co2_mutex = xSemaphoreCreateMutex();
    if(co2_data_queue == NULL)
    {
        ESP_LOGE("CO2", "Error creating co2 data queue");
    }
    if(co2_mutex == NULL)
    {
        ESP_LOGE("CO2", "Error creating co2 mutex");
    }

    while(1) // continuous task loop
    {
        esp_err_t err = ESP_FAIL;
        uint16_t co2_concentration = 0;


        err = i2c_master_transmit(i2c_co2_device_handle, co2_start_cmd, sizeof(co2_start_cmd), pdMS_TO_TICKS(100));
        if(err != ESP_OK)
        {
            ESP_LOGE("CO2 TRANSMIT", "Error writing measure command to sensor with error: 0x%03x", err);
        }

        // after sensors receive initial command, give time to take measurement
        vTaskDelay(pdMS_TO_TICKS(5000));
               
        if(co2_read_data(&co2_concentration) == ESP_OK)
        {
            ESP_LOGI("CO2 Reading", "PPM: %d", co2_concentration);
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
        
}