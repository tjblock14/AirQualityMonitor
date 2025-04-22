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
uint8_t cursor_to_second_row_cmd[2] = {254, 128+64};

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

/*********************************
 * @brief Sends command to display to clear the screen, checks if command successfylly received
 ********************************/
void clear_display_screen()
{
    esp_err_t err = ESP_FAIL;
    err = i2c_master_transmit(i2c_display_device_handle, clear_display_cmd, sizeof(clear_display_cmd), pdMS_TO_TICKS(500));
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error clearing the screen");
    }
}

/*********************************
 * @brief Sends command to display to set the curso to the second row, posiiton "0"
 ********************************/
void move_cursor_to_second_row()
{
    esp_err_t err = ESP_FAIL;
    err = i2c_master_transmit(i2c_display_device_handle, cursor_to_second_row_cmd, sizeof(cursor_to_second_row_cmd), pdMS_TO_TICKS(100));
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error moving cursor to second row");
    }
}

void startup_screen_init()
{
    clear_display_screen();
    reset_text_buffers();
    snprintf(display_text_buf_line1, sizeof(display_text_buf_line1), "Taking initial");
    snprintf(display_text_buf_line2, sizeof(display_text_buf_line2), "measurements");
    
    i2c_master_transmit(i2c_display_device_handle, (uint8_t *)display_text_buf_line1, strlen(display_text_buf_line1), pdMS_TO_TICKS(100));
    move_cursor_to_second_row();
    i2c_master_transmit(i2c_display_device_handle, (uint8_t *)display_text_buf_line2, strlen(display_text_buf_line2), pdMS_TO_TICKS(100));

}

void temp_humid_screen_init()
{
    esp_err_t err = ESP_FAIL;
    clear_display_screen();

    reset_text_buffers();
    sprintf(display_text_buf_line1, "Temp: %dF", sensor_data_buffer.average_temp);
    sprintf(display_text_buf_line2, "Humid: %d%%rH", sensor_data_buffer.average_humidity);

    err = i2c_master_transmit(i2c_display_device_handle, (uint8_t *)display_text_buf_line1, strlen(display_text_buf_line1), pdMS_TO_TICKS(100));
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error sending temperature string to screen");
    }

    // Move cursor to second row
    move_cursor_to_second_row();

    err = i2c_master_transmit(i2c_display_device_handle, (uint8_t *)display_text_buf_line2, strlen(display_text_buf_line2), pdMS_TO_TICKS(100));
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error sending humidity string to screen");
    }
}

void co2_screen_init()
{
    esp_err_t err = ESP_FAIL;
    // Clear screen
    clear_display_screen();

    reset_text_buffers();
    sprintf(display_text_buf_line1, "CO2: %d ppm", sensor_data_buffer.average_co2);

    err = i2c_master_transmit(i2c_display_device_handle, (uint8_t *)display_text_buf_line1, strlen(display_text_buf_line1), pdMS_TO_TICKS(500));
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error sending CO2 string to screen: 0x%03X", err);
    }
}

void voc_screen_init()
{
    esp_err_t err = ESP_FAIL;

    // Clear screen and text buffers before writing anything
    clear_display_screen(); 
    reset_text_buffers();

    sprintf(display_text_buf_line1, "VOC Index:");
    sprintf(display_text_buf_line2, "%d ppb", sensor_data_buffer.average_voc);
    err = i2c_master_transmit(i2c_display_device_handle, (uint8_t *)display_text_buf_line1, strlen(display_text_buf_line1), pdMS_TO_TICKS(100));
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error sending VOC string to screen");
    }

    // Move cursor to second row
    move_cursor_to_second_row();

    err = i2c_master_transmit(i2c_display_device_handle, (uint8_t *)display_text_buf_line2, strlen(display_text_buf_line2), pdMS_TO_TICKS(100));
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error sending humidity string to screen");
    }
}

void battery_screen_init()
{
    esp_err_t err = ESP_FAIL;

    // Clear screen and text buffers before writing anything
    clear_display_screen(); 
    reset_text_buffers();
}

void set_co2_thresh_screen_init()
{
    esp_err_t err = ESP_FAIL;

    // Clear screen and text buffers before writing anything
    clear_display_screen(); 
    reset_text_buffers();

    sprintf(display_text_buf_line1, "CO2 Thresh:");
    sprintf(display_text_buf_line2, "   %d ppm", sensor_data_buffer.co2_user_threshold);
    err = i2c_master_transmit(i2c_display_device_handle, (uint8_t *)display_text_buf_line1, strlen(display_text_buf_line1), pdMS_TO_TICKS(100));


    // Move cursor to second row
    move_cursor_to_second_row();

    err = i2c_master_transmit(i2c_display_device_handle, (uint8_t *)display_text_buf_line2, strlen(display_text_buf_line2), pdMS_TO_TICKS(100));
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error sending CO2 thresh");
    }
}

void set_voc_thresh_screen_init()
{
    esp_err_t err = ESP_FAIL;

    // Clear screen and text buffers before writing anything
    clear_display_screen(); 
    reset_text_buffers();

    sprintf(display_text_buf_line1, "VOC Thresh:");
    sprintf(display_text_buf_line2, "   %d ppb", sensor_data_buffer.voc_user_threshold);
    err = i2c_master_transmit(i2c_display_device_handle, (uint8_t *)display_text_buf_line1, strlen(display_text_buf_line1), pdMS_TO_TICKS(100));

    // Move cursor to second row
    move_cursor_to_second_row();

    err = i2c_master_transmit(i2c_display_device_handle, (uint8_t *)display_text_buf_line2, strlen(display_text_buf_line2), pdMS_TO_TICKS(100));
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error sending VOC thresh to screen");
    }
}

void error_screen_init()
{
    esp_err_t err = ESP_FAIL;

    // Clear screen and text buffers before writing anything
    clear_display_screen(); 
    reset_text_buffers();

    sprintf(display_text_buf_line1, "VOC Thresh:");
    err = i2c_master_transmit(i2c_display_device_handle, (uint8_t *)display_text_buf_line1, strlen(display_text_buf_line1), pdMS_TO_TICKS(100));
}