#include "sensor_tasks.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "string.h"
#include "driver/ledc.h"

uint16_t received_co2_data[10];
uint16_t average_co2; 

void calculate_average_co2()
{
    uint16_t sum = 0; 
    average_co2 = 0;

    for(uint8_t i = 0; i < (sizeof(received_co2_data) / sizeof(received_co2_data[0])); i++)
    {
        sum += received_co2_data[i];
    }

    //average out the arrray. Diving by the total array size /  size of one array value gives the amount of values in array
    average_co2 = sum / (sizeof(received_co2_data) / sizeof(received_co2_data[0]));
}


/***
 * 
 * @brief This task is responsible for handling the co2 values in the queue so that it can be output to the web UI
 * 
 * NOTE: This is last prioritiy. Will only get to this is everything else is working on time
 */
void web_ui_get_data_task(void *parameter)
{
    while(1)
    {
        //wait until the queue is full to average the data
        while(uxQueueSpacesAvailable(co2_data_queue) != 0)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        if(xSemaphoreTake(co2_mutex, pdMS_TO_TICKS(5)) == pdTRUE)
        {
            //reset all data to zero upon new iteration
            memset(&received_co2_data, 0, sizeof(received_co2_data));
            average_co2 = 0;

            for(uint8_t i = 0; i < (sizeof(received_co2_data) / sizeof(received_co2_data[0])); i++)
            {
                if(xQueueReceive(co2_data_queue, &received_co2_data[i], pdMS_TO_TICKS(5)) != pdTRUE)
                {
                    ESP_LOGE("CO2", "Could not receive data from queue");
                }
            }

            //once the values have been read and stored from the queue, calculate the average co2 from the last ten readings
            calculate_average_co2();

            printf("average co2: %d\n", average_co2);

            if(average_co2 > 900)
            {
                ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 4096,0);
            }

            vTaskDelay(pdMS_TO_TICKS(2000));
            ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0,0);

            xSemaphoreGive(co2_mutex);
        }

    }
}

uint16_t get_avg_co2_value()
{
    return average_co2;
}