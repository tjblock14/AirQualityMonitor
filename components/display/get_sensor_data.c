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
 * @brief This function is called after 10 data readings of a sensor has been completed and all of the data has been taken out of the queue
 * @param sensor_data_buf is the specific sensor data buffer array
 * @param sensor_buf_size is the amount of values in the array to average, should always be 10
 * @returns the average value of the last 10 sensor readings
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
 * @brief This function will be called from the display task with parameters, depending on the sensor screen active
 * @param sensor_data is a pointer to the specific sensor data array that contains the 10 values to average
 * @param sensor_index is a pointer to the specific sensor index
 * @param sensor_name is a string identifier of which sensor data is being read
 * @returns the average value for the display, then clears out the memory of the 10 readings
 **************************************/
uint16_t get_average_sensor_data(uint16_t *sensor_data, uint8_t *sensor_index, const char *sensor_name)
{
    uint16_t average_value_for_display = 0;
        average_value_for_display = calculate_average_sensor_value(sensor_data, MAX_SENSOR_READINGS);
        printf("Average %s : %d\r\n", sensor_name, average_value_for_display);

        memset(sensor_data, 0, MAX_SENSOR_READINGS * sizeof(uint16_t));
        *sensor_index = 0;

    return average_value_for_display;
}