#include "i2c_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "general_sensors.h"
#include "temp_sensor.h"
#include "co2_sensor.h"
#include "voc_sensor.h"
#include "get_sensor_data.h"
#include "wifi.h"
#include "esp_web_server.h"
#include "esp_err.h"
#include "esp_log.h"
#include "iaq_ui.h"
#include "user_control.h"
#include "driver/i2c.h"
#include "Userbuttons.h"

//QueueHandle_t temp_readings_queue, humid_readings_queue, voc_readings_queue, co2_readings_queue;

static const char *TAG = "main";

display_screen_pages_t current_page = STARTUP_SCREEN;

void app_main()
{
    // Initialize the SDA and SCL lines for I2C communication
    // This function also currently initializes the PWM signal for the buzzer
    i2c_master_config();
    button_init();

    // Initialize a Wi-Fi connection
    // wifi_init_sta();  
    // start_webserver(); 


    //initialize tasks
    xTaskCreate(temp_humidity_task, "TEMP_HUMIDITY_TASk", 1024 * 3, NULL, 5, NULL);
    xTaskCreate(co2_task, "CO2_TASK", 1024 * 3, NULL, 5, NULL);
    //xTaskCreate(voc_task, "VOC_TASK", 1024 * 3, NULL, 5, NULL);
    xTaskCreate(display_task, "DISPLAY_TASK", 1024 * 3, NULL, 5, NULL);

    xTaskCreate(user_button_task, "BUTTON_TASK", 1024, NULL, 6, NULL);
   
}