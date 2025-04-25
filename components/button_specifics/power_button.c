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

// Variables stored that keep track of is buzzers were acknowledged for deep sleep purposes
RTC_DATA_ATTR bool user_buzzer_acked = false;
RTC_DATA_ATTR bool safety_buzzer_acked = false;

/************
 * @brief The four functions below are used to keep track of whether or not the two buzzers have been acknowledged by the user
 *        if a threshold has been exceeded. Two of the functions reset the ack to false when the levels return below the threshold so that
 *        the next time the threshold is reached, it is seen as unacked
 */
bool has_safety_buzzer_been_acked()
{
    return safety_buzzer_acked;
}

void reset_safety_buzzer_ack()
{
    safety_buzzer_acked = false;
}

bool has_user_buzzer_been_acked()
{
    return user_buzzer_acked;
}

void reset_user_buzzer_ack()
{
    user_buzzer_acked = false;
}

/***
 * @brief This function is implemented for better readability in the handle power button function.
 *  This function checks if either buzzer is on, and if so and the power button is pressed, it will acknowledge
 *  that alarm
 */
void check_if_either_buzzer_on()
{
    if(is_user_buzzer_on())
    {
        user_buzzer_acked = true;
    }
    if(is_safety_buzzer_on())
    {
        safety_buzzer_acked = true;
    }
}


/************
 * @brief This function determines what to do when the power button of the device is pressed
 */
void handle_pwr_btn_press()
{
    bool held_for_ten_seconds = false;
    //held_for_ten_seconds = was_button_held_for_ten_seconds(PWR_BTN_PIN);

    // Check if either buzzer is on, if so, a power button press will acknowledge the alarm, and turn the buzzer off
    check_if_either_buzzer_on();


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