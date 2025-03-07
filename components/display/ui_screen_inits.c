#include "ui_screen_inits.h"
#include "iaq_ui.h"
#include "driver/i2c.h"
#include "i2c_config.h"
#include "esp_err.h"
#include "esp_log.h"
#include "string.h"
#include "get_sensor_data.h"
#include "general_sensors.h"
#include "co2_sensor.h"
#include "voc_sensor.h"
#include "temp_sensor.h"

static const char *TAG = "DISPLAY";

char display_text_buf_line1[16];
char display_text_buf_line2[16];
char display_text_buf_line3[16];
char display_text_buf_line4[16];

/***************
 * @brief Re-initalize all of the text lines to zero
 */
void reset_text_buffers()
{
    memset(display_text_buf_line1, 0, sizeof(display_text_buf_line1));
    memset(display_text_buf_line2, 0, sizeof(display_text_buf_line2));
    memset(display_text_buf_line3, 0, sizeof(display_text_buf_line3));
    memset(display_text_buf_line4, 0, sizeof(display_text_buf_line4));
}

void startup_screen_init()
{
    reset_text_buffers();
    snprintf(display_text_buf_line1, sizeof(display_text_buf_line1), "Taking initial");
    snprintf(display_text_buf_line2, sizeof(display_text_buf_line2), "measurements");
    
    i2c_master_transmit(i2c_display_device_handle, (uint8_t *)display_text_buf_line1, strlen(display_text_buf_line1), pdMS_TO_TICKS(100));
    i2c_master_transmit(i2c_display_device_handle, (uint8_t *)display_text_buf_line2, strlen(display_text_buf_line2), pdMS_TO_TICKS(100));

}

void temp_humid_screen_init()
{
    reset_text_buffers();
    //snprintf(display_text_buf_line1, sizeof(display_text_buf_line1), "Temperature: %dF");
    //snprintf(display_text_buf_line2, sizeof(display_text_buf_line2), "Humidity: %d%%rH");

    i2c_master_transmit(i2c_display_device_handle, (uint8_t *)display_text_buf_line1, strlen(display_text_buf_line1), pdMS_TO_TICKS(100));
    i2c_master_transmit(i2c_display_device_handle, (uint8_t *)display_text_buf_line2, strlen(display_text_buf_line2), pdMS_TO_TICKS(100));

}

void co2_screen_init()
{
    esp_err_t err = ESP_FAIL;
    // get average CO2 level from last 10 readings
    // This funciton blocks until the co2 data queue is full
    uint16_t co2_level = get_average_sensor_data(co2_data_queue, co2_mutex, TAG);

    // Clear screen
    err = i2c_master_transmit(i2c_display_device_handle, clear_display_cmd, sizeof(clear_display_cmd), pdMS_TO_TICKS(500));
    if(err == ESP_OK)
    {
        ESP_LOGI(TAG, "Cleared Screen");
    }
    else
    {
        ESP_LOGE(TAG, "Error clearing the screen");
        ESP_LOGW(TAG, "Error is 0x%02X", err);
    }
    
    vTaskDelay(pdMS_TO_TICKS(100));

    reset_text_buffers();
    sprintf(display_text_buf_line1, "CO2: %d ppm", co2_level);

    err = i2c_master_transmit(i2c_display_device_handle, (uint8_t *)display_text_buf_line1, strlen(display_text_buf_line1), pdMS_TO_TICKS(500));
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error sending string to screen: 0x%03X", err);
    }
    else
    {
        ESP_LOGI(TAG, "Displayed: %s", display_text_buf_line1);
    }
}

void voc_screen_init()
{

}

void battery_screen_init()
{

}

void set_co2_thresh_screen_init()
{

}

void set_voc_thresh_screen_init()
{

}

void error_screen_init()
{
    
}