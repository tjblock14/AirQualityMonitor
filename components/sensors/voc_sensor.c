#include "stdint.h"
#include "string.h"
#include "esp_err.h"
#include "esp_log.h"
#include "i2c_config.h"
#include "general_sensors.h"
#include "temp_sensor.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "sensirion_gas_index_algorithm.h"
#include "esp_sleep.h"

#define VOC_SENS_ADDR      0x59

static const char *TAG = "VOC";
SemaphoreHandle_t voc_mutex = NULL;
uint8_t received_data[6];
uint8_t voc_read_command[8] = {0x26, 0x0F};  // Set first two bytes, they are constant, rest will be copied upon receiving temp/humid data

// The voc algorithm requires 24 hours of data to be fully accurate
RTC_DATA_ATTR GasIndexAlgorithmParams voc_params;
RTC_DATA_ATTR int32_t voc_state0;  // These next three variables are needed to resume algorithm from state before deep sleep 
RTC_DATA_ATTR int32_t voc_state1;  
RTC_DATA_ATTR bool voc_states_valid = false;


/***************
 * @brief This function handles the conversion of the raw data to an indexed value from the sensirion funciton.
 *        It then stores that index in the array that holds onto 10 subsequent readings before averaging and increments the reading index of the sensor
 * @param raw_data is the raw data read from the sensor
 */
void get_voc_index_and_store(uint8_t raw_data[3])
{
    int32_t voc_index_value = 0;
    // Need to convert raw data to 32 bit value for the function
    int32_t sensor_raw_32 = (int32_t)((raw_data[0] << 8) | (raw_data[1]));

    printf("Raw voc sensor value: %ld\r\n", sensor_raw_32);

    // Put data through the algorithm, return parameter of voc_index_value will contain the index
    GasIndexAlgorithm_process(&voc_params, sensor_raw_32, &voc_index_value);

    if(sensor_data_buffer.voc_reading_index < MAX_SENSOR_READINGS)
    {
        sensor_data_buffer.voc_measurement[sensor_data_buffer.voc_reading_index] = voc_index_value;
        sensor_data_buffer.voc_reading_index++;

        ESP_LOGE(TAG, "VOC Index is: %ld", voc_index_value);
    }
}

/*********************************
 * @brief This function is called prior to sending the command to the VOC sensor to take a reading. This function
 *        receives the raw data string from the temperature and humidity sensor in a queue, and then organizes the 
 *        bytes of raw data into the command to be sent to the VOC sensor so that it takes the measured temperature 
 *        humidity data into consideration
 *********************************/
void get_full_voc_command()
{
    // Reset last six bytes of command
    memset(&voc_read_command[2], 0, 6);
    // Reset array to zero
    memset(&received_data, 0, sizeof(received_data));

    while(xQueueReceive(temp_humid_voc_queue, &received_data, pdMS_TO_TICKS(50)) != pdPASS)
    {
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    // Append the received data to the VOC command string
    /* VOC Sensor needs the three humidity bytes first, and three temperature bytes next
    *  but the temperature sensor returns it in the opposite direction we flip them with the mem copies below
    */
    memcpy(&voc_read_command[2], &received_data[3], 3);  // Puts the three humidity bytes in bytes 2,3,4 of command
    memcpy(&voc_read_command[5], &received_data[0], 3);  // Puts temperature bytes in byte 5,6,7 of command
}

void voc_task(void *parameter)
{

    voc_mutex = xSemaphoreCreateMutex();
    if(voc_mutex == NULL)
    {
        ESP_LOGE(TAG, "Error creating VOC mutex");
    }

    esp_sleep_wakeup_cause_t reason_for_wakeup = esp_sleep_get_wakeup_cause();
    if(reason_for_wakeup == ESP_SLEEP_WAKEUP_UNDEFINED || !voc_states_valid)  // Undefined means wakeup reason was not from exiting deep sleep (reboot. etc.)
    {
        // Re-initialize this algorirhtm on fresh startup with no stored data
        GasIndexAlgorithm_init(&voc_params, GasIndexAlgorithm_ALGORITHM_TYPE_VOC);
        ESP_LOGI(TAG, "VOC Index Algorithm initialized");
    }
    else  // wake-up from deep sleep
    {
        GasIndexAlgorithm_init(&voc_params, GasIndexAlgorithm_ALGORITHM_TYPE_VOC);
        GasIndexAlgorithm_set_states(&voc_params, voc_state0, voc_state1);
        ESP_LOGI(TAG, "VOC Index Algorithm successfully restored after deep sleep");
    }
    
    while(1)
    {
        uint8_t idle_voc_cmd[2] = {0x36, 0x15};
        uint8_t voc_raw_data[3] = {0};
        esp_err_t err = ESP_FAIL;
        if(xSemaphoreTake(voc_mutex, pdMS_TO_TICKS(1000)) == pdTRUE)
        {
            // Allows all tasks to take their mutex upon startup
            vTaskDelay(pdMS_TO_TICKS(300));

            // Retrieve the data from the temperature and humidity sensor and get the full VOC command
            get_full_voc_command();

            // Send command to VOC sensor to take measurement and wait for response
            err = i2c_master_transmit(i2c_voc_device_handle, voc_read_command, sizeof(voc_read_command), pdMS_TO_TICKS(500));
            if(err != ESP_OK)
            {
                ESP_LOGE(TAG, "Error sending write command to VOC sensor");
            }

            vTaskDelay(pdMS_TO_TICKS(30));
            
            err = i2c_master_receive(i2c_voc_device_handle, voc_raw_data, sizeof(voc_raw_data), pdMS_TO_TICKS(500));
            if(err != ESP_OK)
            {
                ESP_LOGE(TAG, "Error sending read command to VOC sensor");
            }
                
            // Make sure the crc check for data equals the received crc from sensor
            if(crc_check(voc_raw_data, 2) == voc_raw_data[2])
            {
                get_voc_index_and_store(voc_raw_data);

                // These next two lines prepare the algorithm for deep sleep, retrieving the states before losing all data
                GasIndexAlgorithm_get_states(&voc_params, &voc_state0, &voc_state1);

                printf("State 0: %ld, \r\n State 1: %ld", voc_state0, voc_state1);
                voc_states_valid = true;
            }

            // Put sensor into idle mode before okaying for Deep Sleep to presrve power
            /************************* 
             * Issue with this command, figure out later if needed
            err = i2c_master_transmit(i2c_voc_device_handle, idle_voc_cmd, sizeof(idle_voc_cmd), pdMS_TO_TICKS(300));
            if(err != ESP_OK)
            {
                ESP_LOGE(TAG, "Error powering down VOC sensor");
            }
            else
            {
                ESP_LOGI(TAG, "Powered down VOC sensor");
            }
            *****************************/
        }
        xSemaphoreGive(voc_mutex);

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}