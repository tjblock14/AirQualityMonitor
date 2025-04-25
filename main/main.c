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
#include "esp_sleep.h"

#define WAKEUP_TIME 5000000  // one second

// Needs to do something with this here, will not work because then on ever wake from deep sleep, current page is startup
RTC_DATA_ATTR display_screen_pages_t current_page = STARTUP_SCREEN;

/*********************************
 * 
 * @brief This task checks to see if all of the semaphores used by the sensors and display are free. If so,
 *        then all sensors have completed their periodic reading and the device can enter deep sleep with a timer wakeup
 * 
 * Will also need to implement button presses etc.
 */
void deep_sleep_monitor_task(void *parameter)
{
    // give time for other tasks to start
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI("DEEP_SLEEP", "Checking if all sensor readings complete.....");

    // Check if all mutexes are free, recent user interaction, and the status of the two buzzers
    while((uxSemaphoreGetCount(temp_humid_mutex) == 0) || (uxSemaphoreGetCount(co2_mutex) == 0) || (uxSemaphoreGetCount(voc_mutex) == 0) || check_recent_user_interaction() || is_user_buzzer_on() || is_safety_buzzer_on())
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // will sleep for 5 seconds then wakeup for more readings
    esp_sleep_enable_timer_wakeup(WAKEUP_TIME);
    ESP_LOGI("DEEP_SLEEP", "Entering Deep Sleep");
    esp_deep_sleep_start();
}

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
    xTaskCreate(voc_task, "VOC_TASK", 1024 * 3, NULL, 5, NULL);
    xTaskCreate(display_task, "DISPLAY_TASK", 1024 * 3, NULL, 4, NULL);
    xTaskCreate(user_button_task, "BUTTON_TASK", 1024 * 4, NULL, 6, NULL);
   
    //Lowest priority task
    xTaskCreate(deep_sleep_monitor_task, "DEEP_SLEEP_MONITOR", 1024 * 2, NULL, 1, NULL);
}