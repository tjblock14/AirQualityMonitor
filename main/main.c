#include "pin_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "sensor_tasks.h"
#include "pin_config.h"
#include "wifi.h"
#include "esp_web_server.h"
#include "esp_err.h"


//QueueHandle_t temp_readings_queue, humid_readings_queue, voc_readings_queue, co2_readings_queue;

//static const char *TAG = "main";

void app_main()
{
    //initialize the SDA and SCL lines for I2C communication
    i2c_master_config();

/*
    //create queues for use in RTOS tasks
    //size may need to be changed, depends how long we wait to update web UI
    temp_readings_queue = xQueueCreate(10, sizeof(float));
    if(temp_readings_queue == NULL)
    {
        ESP_LOGE(TAG, "Error creating temperature queue");
    }
    
    humid_readings_queue = xQueueCreate(10, sizeof(float));
    if(humid_readings_queue == NULL)
    {
        ESP_LOGE(TAG, "Error creating humidity queue");
    }

    voc_readings_queue = xQueueCreate(10, sizeof());
    if(voc_readings_queue == NULL)
    {
        ESP_LOGE(TAG, "Error creating voc queue");
    }

    co2_readings_queue = xQueueCreate(10, sizeof());
    if(co2_readings_queue == NULL)
    {
        ESP_LOGE(TAG, "Error creating co2 queue");
    }

    //initialize tasks
    xTaskCreate(temp_humidity_task, "Temp_Humidity_Task", 1024 * 3, NULL, 5, NULL);
    xTaskCreate(co2_task, "CO2_Task", 1024 * 3, NULL, 5, NULL);
    xTaskCreate(voc_task, "VOC_TASK", 1024 * 3, NULL, 5, NULL);
*/

    //setting up wifi and web ui from chatgpt example
    wifi_init_sta();  // Initialize Wi-Fi and connect to the network
    start_webserver();  // Start the web server to serve the data
}