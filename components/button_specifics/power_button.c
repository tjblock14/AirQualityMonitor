#include "power_button.h"
#include "stdbool.h"
#include "freertos/FreeRTOS.h"
#include "driver/i2c.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "Userbuttons.h"
#include "user_control.h"
#include "i2c_config.h"
#include "iaq_ui.h"
#include "general_sensors.h"
#include "ui_screen_inits.h"
#include "i2c_config.h"

#define TEN_SECOND_HOLD   (10000000)  // 10 seconds in microseconds
#define THREE_SECOND_HOLD (3000000)   // 3 seconds in mircoseconds

static const char *TAG = "POWER_BTN";

// Variables stored that keep track of is buzzers were acknowledged for deep sleep purposes
RTC_DATA_ATTR bool user_buzzer_acked = false;
RTC_DATA_ATTR bool safety_buzzer_acked = false;

/************************************************************
 * @brief Two of the four functions below are used to keep track of whether or not the two buzzers have been acknowledged by the user
 *        if a threshold has been exceeded. The two reset functions reset the ack to false when the levels return below the threshold so that
 *        the next time the threshold is breached, it is seen as unacked and the buzzer will be turned on
 ***********************************************************/
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

/***********************************************************
 * @brief This function is implemented for better readability in the handle power button function.
 *  This function checks if either buzzer is on, and if so, the alarm will be3 acknowledged when the power button is pressed
 *************************************************************/
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

/*******************************************
 * @brief This function checks if the power button has been held for ten seconds.
 * @returns true if it has been held for at least 10 seconds, otherwise returns false
 ******************************************/
bool pwr_held_for_ten_seconds(uint32_t press_time)
{
    uint32_t current_time = esp_timer_get_time();
    // While the button is still held and it has not yet been ten seconds, continue delaying
    while((gpio_get_level(PWR_BTN_PIN) == 0) && (current_time < (press_time + TEN_SECOND_HOLD)))
    {
        vTaskDelay(pdMS_TO_TICKS(100));
        current_time = esp_timer_get_time();
    }

    if(current_time >= (press_time + TEN_SECOND_HOLD))
    {
        set_powering_down_screen();

        // Wait until button is released to return true. This prevents the device instantly powering back on if this was not implemented
        while(gpio_get_level(PWR_BTN_PIN) == 0)
        {
            vTaskDelay(pdMS_TO_TICKS(50));
        }
        return true;
    }

    return false;
}


/**************************************
 * @brief This function checks if the power button has been held for three seconds. 
 * @returns true if it has been held for at least 3 seconds. If not, it returns false
 **************************************/
bool pwr_held_for_three_seconds(uint32_t press_time)
{
    uint32_t current_time = esp_timer_get_time();
    // While the button is still held and it has not yet been three seconds, continue delaying
    while((gpio_get_level(PWR_BTN_PIN) == 0) && (current_time < (press_time + THREE_SECOND_HOLD)))
    {
        vTaskDelay(pdMS_TO_TICKS(100));
        current_time = esp_timer_get_time();
    }

    if(current_time >= (press_time + THREE_SECOND_HOLD))
    {
        return true;
    }

    return false;
}


/******************************************
 * @brief This function determines what to do when the power button of the device is pressed
 *        It chekcs if a buzzer is currently on. If a buzzer is on, a press of this button will turn it off. 
 *        It also checks if the button has been held for either 10 seconds or 3 seconds. If either of these is true, it proceeds accordingly
 ******************************************/
void handle_pwr_btn_press()
{
    // Used in ten second and three second hold checks
    uint32_t press_time = esp_timer_get_time();

    check_if_either_buzzer_on();

    if(pwr_held_for_three_seconds(press_time) && !pwr_held_for_ten_seconds(press_time)) // If held for at least three seconds and not 10, put to sleep
    {
        power_down_display();
        vTaskDelay(pdMS_TO_TICKS(100));
        esp_sleep_enable_ext0_wakeup(PWR_BTN_PIN, 0);          // Wake-up from deep sleep when the power button is pressed
        esp_sleep_enable_timer_wakeup(pdMS_TO_TICKS(5000));    // If no button wsa pressed, wake up after 5 seocnds to take measurement, then it proceeds with normal sleep cycle
        ESP_LOGI("DEEP_SLEEP", "Entering Deep Sleep");
        esp_deep_sleep_start();
    }
    else if(pwr_held_for_ten_seconds(press_time)) // If held for 10 seconds, power down
    {
        ESP_LOGI(TAG, "Long Press of 10s detected, powering off");

        // Turn display backlight off
        power_down_display();
        vTaskDelay(pdMS_TO_TICKS(100));

        // Enable the wakeup from pressing the power button
        esp_sleep_enable_ext0_wakeup(PWR_BTN_PIN, 0);

        esp_deep_sleep_start();
        ESP_LOGI("DEEP_SLEEP", "Entering deep sleep until pwr button pressed");
    }
}