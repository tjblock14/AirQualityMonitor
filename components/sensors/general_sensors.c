#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_err.h"
#include "Userbuttons.h"
#include "power_button.h"
#include "general_sensors.h"
#include "i2c_config.h"
#include "esp_check.h"
#include "co2_sensor.h"
#include "voc_sensor.h"

#define CRC_INIT          0xFF
#define CRC_POLYNOMIAL    0x31

// Variables used to track whether or not the buzzers or on
bool user_buzzer_status = false;
bool safety_buzzer_status = false;

// create an instance of this struct to be used 
// RTC_DATA_ATTR will make sure this struct is not lost during deep sleep so it holds onto all readings
RTC_DATA_ATTR sensor_readings_t sensor_data_buffer = {
    .co2_generally_unsafe_value = 2000,
    .co2_user_threshold         = 1500,
    .voc_user_threshold         = 300, 
    .voc_generally_unsafe_value = 660
};


/***************
 * @brief CRC function for all sensors
 * @param data the data read from the sensor
 * @param count amount of bytes
 ***************/
uint8_t crc_check(const uint8_t* data, uint16_t count) 
{
    uint16_t current_byte;
    uint8_t crc = CRC_INIT;
    uint8_t crc_bit;
    /* calculates 8-Bit checksum with given polynomial */
    for (current_byte = 0; current_byte < count; ++current_byte) 
    {
        crc ^= (data[current_byte]);
        for (crc_bit = 8; crc_bit > 0; --crc_bit) 
        {
            if (crc & 0x80)
                crc = (crc << 1) ^ CRC_POLYNOMIAL;
            else
                crc = (crc << 1);
        }
    }
    return crc;
}

/*****************
 * @brief The two functions below are used by the deep sleep task to check the status of the two buzzers
 * If either one of these functions returns true (either buzzer is on), the device will avoid entering deep sleep
 ********************/
bool is_safety_buzzer_on()
{
    return safety_buzzer_status;
}

bool is_user_buzzer_on()
{
    return user_buzzer_status;
}

// Function to turn the user threshold buzzer off since it can be used in two places
void turn_user_buzzer_off()
{
    esp_err_t err = ESP_FAIL;
    err = ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0,0);
    if(err != ESP_OK)
    {
        ESP_LOGE("BZR", "Error turning buzzer off: %s", esp_err_to_name(err));
    }
    user_buzzer_status = false;
}

/****************************************
 * @brief compare measured average value against the generally unsafe values, if it has been exceeded, turn on large buzzer
 ****************************************/
void check_general_safety_value()
{
    if(((sensor_data_buffer.average_voc > sensor_data_buffer.voc_generally_unsafe_value)|| (sensor_data_buffer.average_co2 > sensor_data_buffer.co2_generally_unsafe_value)) && !has_safety_buzzer_been_acked())
    {
        gpio_set_level(LARGE_BUZZER_PIN, 1);
        safety_buzzer_status = true;
    }
    // Levels returned below threshold, and the buzzer currently is acked
    else if(!((sensor_data_buffer.average_voc > sensor_data_buffer.voc_generally_unsafe_value) || (sensor_data_buffer.average_co2 > sensor_data_buffer.co2_generally_unsafe_value)) && has_safety_buzzer_been_acked())
    {
        // turn buzzer off since levels returned below threshold, and then unack it so that next time the threshold is reached, it will not already be acked
        gpio_set_level(LARGE_BUZZER_PIN, 0);
        reset_safety_buzzer_ack();
    }
    else // Levels are still above threshold, but buzzer has been acked
    {
        gpio_set_level(LARGE_BUZZER_PIN, 0);

    }
}

/*********************************
 * @brief If either the CO2 or VOC user-set threshold has been reached or exceeded, and the buzzer has not yet been acknowledged by the user, turn the buzzer on
 ********************************/
void check_user_threshold()
{
    esp_err_t err = ESP_FAIL;
    // Compare measured values to thresholds, and if it has been exceeded, only proceed if the buzzer has not yet been acknowledged
    if(((sensor_data_buffer.average_voc > sensor_data_buffer.voc_user_threshold) || (sensor_data_buffer.average_co2 > sensor_data_buffer.co2_user_threshold)) && !has_user_buzzer_been_acked())
    {
        err = ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 4096,0);  // 50% duty cycle
        if(err != ESP_OK)
        {
            ESP_LOGE("BZR", "Error turning buzzer on: %s", esp_err_to_name(err));
        }
        user_buzzer_status = true;
    }
    // else if the buzzers have returned below their threshold, and the buzzer is still acked, reset  the ack
    else if(!((sensor_data_buffer.average_voc > sensor_data_buffer.voc_user_threshold) || (sensor_data_buffer.average_co2 > sensor_data_buffer.co2_user_threshold)) && has_user_buzzer_been_acked())
    { 
        reset_user_buzzer_ack();
        turn_user_buzzer_off();
    }
    else // Levels are still above threshold, but buzzer has been acked
    {
        turn_user_buzzer_off();
    }
}