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

#define WAKEUP_TIME 5000000  // five seconds

// Needs to do something with this here, will not work because then on ever wake from deep sleep, current page is startup
RTC_DATA_ATTR display_screen_pages_t current_page = STARTUP_SCREEN;

/*********************************
 * @brief This task checks to see if all of the mutexes used by the sensors and display are free, as well as if any buzzers are on, and if there
 *        has been any recent user interaction. If so, then the device can sleep for 5 seconds
 */
void deep_sleep_monitor_task(void *parameter)
{
    // Allow all other tasks to begin
    vTaskDelay(pdMS_TO_TICKS(1000));

    // Check if any of the sensor mutexes are taken, if either buzzer is on, and if there has been recent user interaction,
    // if none of these things are true, move on, otherwise delay and check again
    ESP_LOGI("DEEP_SLEEP", "Checking if device is ready for deep sleep.....");
    while((uxSemaphoreGetCount(temp_humid_mutex) == 0) || (uxSemaphoreGetCount(co2_mutex) == 0) || (uxSemaphoreGetCount(voc_mutex) == 0) || \
           check_recent_user_interaction() || is_user_buzzer_on() || is_safety_buzzer_on())
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // Turn backlight off of the display
    if(!is_display_off_in_consistent_sleep())
    {
        power_down_display();
    }
    vTaskDelay(pdMS_TO_TICKS(700));
    esp_sleep_enable_ext0_wakeup(PWR_BTN_PIN, 0);  // Wake-up from deep sleep when the power button is pressed
    esp_sleep_enable_timer_wakeup(WAKEUP_TIME);    // Wake up after 5 seconds to take periodic measurements
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
    xTaskCreate(display_task, "DISPLAY_TASK", 1024 * 4, NULL, 4, NULL);
    xTaskCreate(user_button_task, "BUTTON_TASK", 1024 * 4, NULL, 6, NULL);
   
    //Lowest priority task
    xTaskCreate(deep_sleep_monitor_task, "DEEP_SLEEP_MONITOR", 1024 * 2, NULL, 1, NULL);
}