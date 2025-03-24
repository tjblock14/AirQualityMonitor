#include "general_sensors.h"
#include "co2_sensor.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "string.h"
#include "driver/ledc.h"
#include "iaq_ui.h"

/******************************
 * @brief This function is called after all of the data has been read from the queue, it then calculates and returns the average
 * @param sensor_data_buf is the specific sensor data buffer array
 * @param sensor_buf_size is the amount of values in the array to average, should always be 10
 *****************************/
uint16_t calculate_average_sensor_value(uint16_t sensor_data_buf[10], size_t sensor_buf_size)
{
    uint16_t sum = 0;

    for(uint8_t i = 0; i < sensor_buf_size; i++)
    {
        sum += sensor_data_buf[i];
    }

    return sum / sensor_buf_size;

}


/***************************************
 * @brief this function will be called from the display task with parameters, depending on the sensor screen active
 * @param sensor_data is a pointer to the specific sensor data array that contains the 10 values to average
 * @param sensor_index is a pointer to the specific sensor index
 * @param sensor_semaphore is the semaphore used to avoid race conditions with the sensor data
 * @param sensor_name is a string identifier of which sensor data is being read
 **************************************/
uint16_t get_average_sensor_data(void *sensor_data, uint8_t *sensor_index, SemaphoreHandle_t sensor_semaphore, const char *sensor_name)
{
    uint16_t average_value_for_display = 0;

    while(*sensor_index < MAX_SENSOR_READINGS)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    if(xSemaphoreTake(sensor_semaphore, pdMS_TO_TICKS(5)) == pdTRUE)
    {
        average_value_for_display = calculate_average_sensor_value(sensor_data, MAX_SENSOR_READINGS);
        printf("Average %s : %d\r\n", sensor_name, average_value_for_display);

        memset(sensor_data, 0, sizeof(sensor_data));
        *sensor_index = 0;
        xSemaphoreGive(sensor_semaphore);
    }
    return average_value_for_display;
}



void check_co2_thresh()
{
    // If the CO2 threshold is reached, turn the buzzer on for two seconds
    if(sensor_data_buffer.average_co2 > 900)
    {
        ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 4096,0);
        vTaskDelay(pdMS_TO_TICKS(2000));
        ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0,0);
    }
}