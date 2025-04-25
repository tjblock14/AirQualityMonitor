#include "power_button.h"
#include "stdbool.h"
#include "freertos/FreeRTOS.h"
#include "driver/i2c.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "Userbuttons.h"
#include "user_control.h"
#include "i2c_config.h"
#include "general_sensors.h"


static const char *TAG = "POWER_BTN";

RTC_DATA_ATTR bool buzzer_acked = false;

bool has_buzzer_been_acked()
{
    return buzzer_acked;
}

void reset_buzzer_ack()
{
    buzzer_acked = false;
}

void handle_pwr_btn_press()
{
    bool held_for_ten_seconds = false;
    //held_for_ten_seconds = was_button_held_for_ten_seconds(PWR_BTN_PIN);
    if(is_buzzer_on())
    {
        buzzer_acked = true;
    }

    if(held_for_ten_seconds)
    {
        ESP_LOGI(TAG, "Long Press of 10s detected, powering off");

        // Enable the wakeup from pressing the power button
        esp_sleep_enable_ext0_wakeup(PWR_BTN_PIN, 0);
        esp_deep_sleep_start();
    }
    else  // I think if not held for 10 seconds, but held for 3, we will put to sleep, need to figure this out still, might just do sleep on press
    {

    }
}