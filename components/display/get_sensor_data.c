#include "general_sensors.h"
#include "co2_sensor.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "string.h"
#include "driver/ledc.h"
#include "iaq_ui.h"

uint16_t received_co2_data[10];
uint16_t received_temp_data[10];
uint16_t received_humid_data[10];

/******************************
 * @brief This function is called after all of the data has been read from the queue, it then calculates and returns the average
 *****************************/
uint16_t calculate_average_sensor_value(uint16_t sensor_data_buf[10], size_t sensor_buf_size)
{
    uint16_t sum = 0;
    uint16_t average_value = 0;

    for(uint8_t i = 0; i < sensor_buf_size; i++)
    {
        sum += sensor_data_buf[i];
    }

    average_value = sum / sensor_buf_size;

    return average_value;
}


/***************************************
 * @brief this function will be called from the display task with parameters, depending on the sensor screen active
 * @param sensor_queue is the queue that holds the most 10 recent reads of the sensor
 * @param sensor_semaphore is the semaphore used to avoid race conditions with the sensor data
 * @param sensor_name is a string identifier of which sensor data is being read
 **************************************/
uint16_t get_average_sensor_data(QueueHandle_t sensor_queue, SemaphoreHandle_t sensor_semaphore, const char *sensor_name)
{
    uint16_t received_sensor_data_from_queue[10] = {0};
    uint16_t average_value_for_display = 0;
    // wait for the queue to fill up with 10 values before averaging it out
    while(uxQueueSpacesAvailable(sensor_queue) != 0)
    {
        // if the queue is not full, delay for one second and then check again
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    if(xSemaphoreTake(sensor_semaphore, pdMS_TO_TICKS(5)) == pdTRUE)
    {
        for(uint8_t i = 0; i < (sizeof(received_sensor_data_from_queue) / sizeof(received_sensor_data_from_queue[0])); i++)
        {
            if(xQueueReceive(sensor_queue, &received_sensor_data_from_queue[i], pdMS_TO_TICKS(5)) != pdTRUE)
            {
                ESP_LOGE(sensor_name, "Error getting data from %s queue", sensor_name);
            }
        }

        average_value_for_display = calculate_average_sensor_value(received_sensor_data_from_queue, (sizeof(received_sensor_data_from_queue) / sizeof(received_sensor_data_from_queue[0])));
        printf("Average %s : %d\r\n", sensor_name, average_value_for_display);
        xSemaphoreGive(sensor_semaphore);
    }
    return average_value_for_display;
}



void check_c02_thresh()
{
    // If the CO2 threshold is reached, turn the buzzer on for two seconds
    if(average_co2 > 900)
    {
        ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 4096,0);
        vTaskDelay(pdMS_TO_TICKS(2000));
        ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0,0);
    }
}