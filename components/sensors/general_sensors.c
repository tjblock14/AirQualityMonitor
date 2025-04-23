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
#include "general_sensors.h"
#include "i2c_config.h"
#include "esp_check.h"
#include "co2_sensor.h"
#include "voc_sensor.h"

#define CRC_INIT          0xFF
#define CRC_POLYNOMIAL    0x31

// create an instance of this struct to be used 
// RTC_DATA_ATTR will make sure this struct is not lost during deep sleep so it holds onto all readings
RTC_DATA_ATTR sensor_readings_t sensor_data_buffer = {
    .co2_generally_unsafe_value = 2000,
    .co2_user_threshold         = 1200,
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

/***********************************************************
 * 
 * *****************************************
 * 
 * *************************************************************/
// Will also want to add in a flag incase the user pressed button to manually turn off alarm

/****************************************
 * @brief compare measured average value against the generally unsafe values, if it has been exceeded, turn on large buzzer
 ****************************************/
void check_general_safety_value()
{
    if((sensor_data_buffer.average_voc > sensor_data_buffer.voc_generally_unsafe_value)|| (sensor_data_buffer.average_co2 > sensor_data_buffer.co2_generally_unsafe_value))
    {
        gpio_set_level(LARGE_BUZZER_PIN, 1);
    }
    else
    {
        gpio_set_level(LARGE_BUZZER_PIN, 0);
    }
}

/*********************************
 * @brief If the user-set thresholds have been exceeded, turn on the quieter of the two buzzers
 ********************************/
void check_user_threshold()
{
    // If the CO2 threshold is reached, turn the buzzer on for two seconds
    if((sensor_data_buffer.average_voc > sensor_data_buffer.co2_user_threshold)|| (sensor_data_buffer.average_co2 > sensor_data_buffer.co2_user_threshold))
    {
        ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 4096,0);
        ESP_LOGI("BZR", "Turned buzzer on");
        vTaskDelay(pdMS_TO_TICKS(2000));
        ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0,0);
    }
}