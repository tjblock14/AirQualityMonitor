#include "temp_sensor.h"
#include "general_sensors.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "i2c_config.h"
#include "stdint.h"
#include "stdbool.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#define TEMP_SENS_ADDR     0x44

static const char *TAG = "TEMP/HUMID";

QueueHandle_t temp_data_queue = NULL;
QueueHandle_t humid_data_queue    = NULL;
SemaphoreHandle_t temp_humid_mutex   = NULL;

/*************************
 * @brief this function takes the raw data from the sensor and converts it into a readable format
 * @param data is the full data array from the sensor read
 * @param temperature is a pointer to the variable where the readable temperature is stored
 * @param humidity is a pointer to the variable where the readable humidity is stored
 * @param temp_unit will determine if the raw temperature data is converted into Celcius or Farenheit, based on user settings
 *************************/
void calculate_readable_temp_humid(uint8_t data[6], uint16_t *temperature, uint16_t *humidity) // will eventually want another parameter, telling if user has device in F or C
{
    uint16_t raw_temp = 0;
    uint16_t raw_humidity = 0;

    // for celsius
    //float temperature = -45 + 175 * (raw_temp / 65535.0);  // Temperature in Celsius

    //for farenheit
    raw_temp = (data[0] << 8) | data[1];
    *temperature = -49 + 315 * (raw_temp / 65535.0);     // Temerature in Farenheit

    raw_humidity = (data[3] << 8) | data[4];
    *humidity = -6 + 125 * (raw_humidity / 65535.0);     // Relative humidity in %
}


// going to want a mutex between temp sensor and voc sensor so voc can take humidity sensitive measurements
void temp_humidity_task(void *parameter)
{
    uint8_t temp_humid_measure_cmd = {0xFD};

    temp_data_queue = xQueueCreate(10, sizeof(uint16_t));
    humid_data_queue       = xQueueCreate(10, sizeof(uint16_t));
    temp_humid_mutex = xSemaphoreCreateMutex();
    if(temp_data_queue == NULL)
    {
        ESP_LOGE(TAG, "Error creating temp data queue");
    }
    if(humid_data_queue == NULL)
    {
        ESP_LOGE(TAG, "Error creating humidity data queue");
    }
    if(temp_humid_mutex == NULL)
    {
        ESP_LOGE(TAG, "Error creating temp/humid mutex");
    }

    while(1)
    {
        esp_err_t err = ESP_FAIL;
        uint8_t sensor_data[6] = {0};
        uint16_t temperature = 0;
        uint16_t humidity = 0;

        if(xSemaphoreTake(temp_humid_mutex, pdMS_TO_TICKS(1000)) == pdTRUE)
        {
            err = i2c_master_transmit(i2c_temp_device_handle, &temp_humid_measure_cmd, sizeof(temp_humid_measure_cmd), pdMS_TO_TICKS(100));
            if(err != ESP_OK)
            {
                ESP_LOGE(TAG, "Error with temp sensor write cmd: 0x%03X", err);
            }
            
            vTaskDelay(pdMS_TO_TICKS(1000));
            err = i2c_master_receive(i2c_temp_device_handle, sensor_data, sizeof(sensor_data), pdMS_TO_TICKS(100));
            if(err != ESP_OK)
            {
                ESP_LOGE(TAG, "Error with temp sensor read cmd");
            }
    
    
            // Ensure CRC is successful for both the temeprature and humidity data before proceeding
            if((crc_check(sensor_data, 2) == sensor_data[2]) && (crc_check(&sensor_data[3], 2) == sensor_data[5]))
            {
                calculate_readable_temp_humid(sensor_data, &temperature, &humidity);
                ESP_LOGW(TAG, "Measured Temperatue: %d\n Measured Humidity: %d", temperature, humidity);
                if(xQueueSend(temp_data_queue, &temperature, pdMS_TO_TICKS(5)) != pdTRUE)
                {
                    ESP_LOGE(TAG, "Error adding temperature data to queue");
                }
                if(xQueueSend(humid_data_queue, &humidity, pdMS_TO_TICKS(5)) != pdTRUE)
                {
                    ESP_LOGE(TAG, "Error adding humidity data to queue");
                }
            }
            else
            {
                continue;
            }
        }
        xSemaphoreGive(temp_humid_mutex);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}