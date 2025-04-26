#include "stdint.h"
#include "string.h"
#include "esp_err.h"
#include "esp_log.h"
#include "i2c_config.h"
#include "general_sensors.h"
#include "temp_sensor.h"
#include "Userbuttons.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_sleep.h"

#define VOC_SENS_ADDR      0x58

static const char *TAG = "VOC";
SemaphoreHandle_t voc_mutex = NULL;
uint8_t init_voc_sensor_cmd[2] = {0x20, 0x03};
uint8_t voc_measure_cmd[2]     = {0x20, 0x08};
uint8_t received_data[6] = {0};

// Variable that tracks if the VOC sensor has already been initialized to avoid unccecesary re-initialization
RTC_DATA_ATTR static bool voc_sensor_initialized = false;

// Simple function for the init command of the sensor since we need to use this repeatedly on first startup
void init_voc_sensor()
{
    esp_err_t err = ESP_FAIL;
    err = i2c_master_transmit(i2c_voc_device_handle, init_voc_sensor_cmd, sizeof(init_voc_sensor_cmd), pdMS_TO_TICKS(200));
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error sending init command to sensor, %s", esp_err_to_name(err));
    }
    else 
    {
        ESP_LOGI(TAG, "initialized VOC sensor");
    }
}

// Function to send the measurement command and receive the VOC Data
// Condensed into a function for easier readability
void measure_voc_sensor()
{
    esp_err_t err = ESP_FAIL;
    err = i2c_master_transmit(i2c_voc_device_handle, voc_measure_cmd, sizeof(voc_measure_cmd), pdMS_TO_TICKS(500));
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to send measure command: %s", esp_err_to_name(err));
    }
    else  // measure command successful, now send
    {
        vTaskDelay(pdMS_TO_TICKS(20));
        err = i2c_master_receive(i2c_voc_device_handle, received_data, sizeof(received_data), pdMS_TO_TICKS(500));
        if(err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to read data: %s", esp_err_to_name(err));
        }
    }
}


void voc_task(void *parameter)
{

    voc_mutex = xSemaphoreCreateMutex();
    if(voc_mutex == NULL)
    {
        ESP_LOGE(TAG, "Error creating VOC mutex");
    }
    
    while(1)
    {
        uint16_t readable_voc = 0;
        // Set data array to zero upon new read
        memset(received_data, 0, sizeof(received_data));
        esp_err_t err = ESP_FAIL;
        if(xSemaphoreTake(voc_mutex, pdMS_TO_TICKS(1000)) == pdTRUE)
        {
            // Allows all tasks to take their mutex upon startup
            vTaskDelay(pdMS_TO_TICKS(200));

            
            // On fresh power up, the sensor needs to initialize, then take 15 consecutive readings before it gets a valid value
            // Also check for recent button press because if button was pressed on startup, device will not enter sleep immediately
            // so we need to ensure there was no press as well
            if(!check_recent_user_interaction() && !voc_sensor_initialized)
            {
                init_voc_sensor();
                voc_sensor_initialized = true;
                vTaskDelay(pdMS_TO_TICKS(50));

                // take 20 readings on fresh startup to be safe and ensure readings area valid when we start storing data
                for(uint8_t i = 0; i < 20; i++)
                {
                    measure_voc_sensor();
                    vTaskDelay(pdMS_TO_TICKS(1000));
                }
            }

            measure_voc_sensor();
            // Make sure the crc check for data equals the received crc from sensor
            if(crc_check(&received_data[3], 2) == received_data[5])
            {
                readable_voc = (received_data[3] << 8) | received_data[4];
                // add the read voc value to the array, and increment to next index for next reading
                if(sensor_data_buffer.co2_reading_index < MAX_SENSOR_READINGS)
                {
                    sensor_data_buffer.voc_measurement[sensor_data_buffer.voc_reading_index] = readable_voc;
                    sensor_data_buffer.voc_reading_index++;
                }

                ESP_LOGI(TAG, "%d ppb", readable_voc);
            }
            else
            {
                ESP_LOGE(TAG, "CRC Mismatch");
            }
        }
        xSemaphoreGive(voc_mutex);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}