#include "i2c_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "sensor_tasks.h"
#include "get_sensor_data.h"
#include "wifi.h"
#include "esp_web_server.h"
#include "esp_err.h"
#include "esp_log.h"
#include "mqtt.h"
#include "aws_setup.h"

#include "driver/i2c.h"

//QueueHandle_t temp_readings_queue, humid_readings_queue, voc_readings_queue, co2_readings_queue;

static const char *TAG = "main";

void app_main()
{
    // Initialize the SDA and SCL lines for I2C communication
    i2c_master_config();

    // Initialize a Wi-Fi connection
    wifi_init_sta();  
    start_webserver(); 

    // Initialize the MQTT for communication with AWS
     //mqtt_init();


    //initialize tasks
    //xTaskCreate(temp_humidity_task, "TEMP_HUMIDITY_TASk", 1024 * 3, NULL, 5, NULL);
    xTaskCreate(co2_task, "CO2_TASK", 1024 * 3, NULL, 5, NULL);
    //xTaskCreate(voc_task, "VOC_TASK", 1024 * 3, NULL, 5, NULL);

    //start web ui update task
    xTaskCreate(web_ui_get_data_task, "WEB UI TASK", 1024*2, NULL, 5, NULL);
   
    // Start AWS task
    //xTaskCreate(aws_task, "AWS_TASK", 1024 * 3, NULL, 4, NULL);
}