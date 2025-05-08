#include "i2c_config.h"
#include "string.h"
#include "iaq_ui.h"
#include "get_sensor_data.h"
#include "general_sensors.h"
#include "co2_sensor.h"
#include "voc_sensor.h"
#include "temp_sensor.h"
#include "ui_screen_inits.h"
#include "Userbuttons.h"
#include "user_control.h"
#include "esp_sleep.h"
#include <stdbool.h>

uint8_t clear_display_cmd[2] = {0x7C, 0x2D};
RTC_DATA_ATTR bool read_inital_data_on_startup = false;

/**************************************
 * @brief This function prepares the display for deep sleep and power off modes
 *        It turns all of the backlights to 0% brightness, and clears the screen
 *************************************/
void power_down_display()
{
    uint8_t blue_backlight_off_cmd[2] = {0x7C, 0xBC};
    uint8_t green_backlight_off_cmd[2] = {0x7C, 0x9E};
    uint8_t primary_backlight_off_cmd[2] = {0x7C, 0x80};

    ESP_ERROR_CHECK(i2c_master_transmit(i2c_display_device_handle, clear_display_cmd, sizeof(clear_display_cmd), pdMS_TO_TICKS(500)));
    ESP_ERROR_CHECK(i2c_master_transmit(i2c_display_device_handle, blue_backlight_off_cmd, sizeof(blue_backlight_off_cmd), pdMS_TO_TICKS(500)));
    ESP_ERROR_CHECK(i2c_master_transmit(i2c_display_device_handle, green_backlight_off_cmd, sizeof(green_backlight_off_cmd), pdMS_TO_TICKS(500)));
    ESP_ERROR_CHECK(i2c_master_transmit(i2c_display_device_handle, primary_backlight_off_cmd, sizeof(primary_backlight_off_cmd), pdMS_TO_TICKS(500)));

    set_display_off_in_sleep();
}

/**************************************
 * @brief This function sends three subseuent commands to the display when it wakes up, setting the
 *        brightness of the three different backlights, primary, blue, and green for the best readability
 *        of things displayed on the screen
 **************************************/
void power_display_on()
{
    uint8_t blue_backlight_on_cmd[2] = {0x7C, 210};
    uint8_t green_backlight_on_cmd[2] = {0x7C, 180};
    uint8_t primary_backlight_on_cmd[2] = {0x7C, 0x9D};

    ESP_ERROR_CHECK(i2c_master_transmit(i2c_display_device_handle, blue_backlight_on_cmd, sizeof(blue_backlight_on_cmd), pdMS_TO_TICKS(500)));
    ESP_ERROR_CHECK(i2c_master_transmit(i2c_display_device_handle, green_backlight_on_cmd, sizeof(green_backlight_on_cmd), pdMS_TO_TICKS(500)));
    ESP_ERROR_CHECK(i2c_master_transmit(i2c_display_device_handle, primary_backlight_on_cmd, sizeof(primary_backlight_on_cmd), pdMS_TO_TICKS(500)));
}

/****************************************
 * @brief This function gets the next page to display on the screen, based on what the current page is
 * @param display_page is the currently displayed page on the screen
 * @returns the page that is to be displayed next
 ***************************************/
display_screen_pages_t get_next_screen_page(display_screen_pages_t displayed_page)
{
    display_screen_pages_t next_screen = STARTUP_SCREEN;
    switch(displayed_page)
    {
        case STARTUP_SCREEN:
            next_screen = TEMPERATURE_HUMIDITY_SCREEN;
            break;
        case TEMPERATURE_HUMIDITY_SCREEN:
            next_screen = CO2_SCREEN;
            break;
        case CO2_SCREEN:
            next_screen = VOC_SCREEN;
            break;
        case VOC_SCREEN:
            next_screen = TEMPERATURE_HUMIDITY_SCREEN;
            break;
        // if on threshold screens, do nothing
        case SET_CO2_THRESH_SCREEN:
            next_screen = SET_CO2_THRESH_SCREEN;
            break;
        case SET_VOC_THRESH_SCREEN:
            next_screen = SET_VOC_THRESH_SCREEN;
            break;
        default:   // If there is some issue, default back to startup screen
            next_screen = STARTUP_SCREEN;
            break;
    }
    return next_screen;
}

/*************************************************
 * @brief This function is used to set the screen based on what the current page is. Depending on the current page, 
 *        the corresponding function is called with the proper strings to display
 * @param set_page is the page that will be displayed on the screen when this function is called
 *************************************************/
void set_ui_screen_page(display_screen_pages_t set_page)
{
    switch(set_page)
    {
        case STARTUP_SCREEN:
            startup_screen_init();
            break;
        case TEMPERATURE_HUMIDITY_SCREEN:
            temp_humid_screen_init();
            break;
        case CO2_SCREEN:
            co2_screen_init();
            break;
        case VOC_SCREEN:
            voc_screen_init();
            break;
        case SET_CO2_THRESH_SCREEN:
            set_co2_thresh_screen_init();
            break;
        case SET_VOC_THRESH_SCREEN:
            set_voc_thresh_screen_init();
            break;
        default:
            error_screen_init();
            break;
    }
}

// This function is used to prevent moving off the Startup Screen before any data is ready on initial startup
bool is_initial_data_ready()
{
    return read_inital_data_on_startup;
}


/*******************************************************
 * @brief This function is called four times in the display task, once for each sensor. It checks if 10 readings of the sensor has been taken and if so, it will get
 *        the average value of the 10 readings and display it is the device is awake and currently on that sensor's screen
 * @param sensor_readings is the array of the 10 most recent readings of the sensor
 * @param reading_index is how many readings of the sensor has been taken. This is needed because if this is not 10 yet, nothing will happen here
 * @param average_value is where the average of the 10 most recent readings for the sensor is stored
 * @param sensor_mutex is the mutex for each specific sensor
 * @param sensor_name is a character string for which sensor is being worked with. This is needed so that upon first startup, once the CO2 sensor is averaged, it will 
 *                    move from the startup screen to the CO2 screen and then the user can interact with the device from there
 * @param sensor_data_screen is the screen related to the sensor where it displays its average value
 *******************************************************/
void process_sensor_data(uint16_t *sensor_readings, uint8_t *reading_index, uint16_t *average_value ,SemaphoreHandle_t sensor_mutex, const char *sensor_name, uint8_t sensor_data_screen)
{
    if(*reading_index >= MAX_SENSOR_READINGS)
    {
        if(xSemaphoreTake(sensor_mutex, pdMS_TO_TICKS(20)) == pdTRUE)
        {
            *average_value = get_average_sensor_data(sensor_readings, reading_index, sensor_name);
            if(current_page == sensor_data_screen && !is_display_off_in_consistent_sleep())
            {
                set_ui_screen_page(current_page);
            }

            if(!strcmp(sensor_name, "CO2"))
            {
                if(!read_inital_data_on_startup)
                {
                    current_page = CO2_SCREEN;
                    set_ui_screen_page(current_page);
                }
                read_inital_data_on_startup = true;
            }

            xSemaphoreGive(sensor_mutex);
        }
    }
}


/*******************
 * @brief The main task for the display. This will read from the data queues of the sensors and display it on the UI
 *******************/
void display_task(void *parameter)
{
    // If the device is first turning on, or woken by user button press, make sure it is at the correct brightness
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    if(wakeup_reason == ESP_SLEEP_WAKEUP_UNDEFINED || wakeup_reason == ESP_SLEEP_WAKEUP_EXT0)
    {
       power_display_on();
       set_backlight_updated(); // Avoid setting the brightness multiple times
    }
    vTaskDelay(pdMS_TO_TICKS(1000));  // Allow time for display to show its brightnesses before sending string

   // On fresh startup, display taking initial measurements
   if(!read_inital_data_on_startup)
   {
    set_ui_screen_page(STARTUP_SCREEN);
   }
   else if(read_inital_data_on_startup && check_recent_user_interaction())  // initial data taken, device is awake
   {
    set_ui_screen_page(CO2_SCREEN);
   }

    while(1)
    {
        // Get most recent average value from all sensors
        process_sensor_data(sensor_data_buffer.co2_concentration, &sensor_data_buffer.co2_reading_index, &sensor_data_buffer.average_co2, co2_mutex, "CO2", CO2_SCREEN);
        process_sensor_data(sensor_data_buffer.temperature, &sensor_data_buffer.temp_reading_index, &sensor_data_buffer.average_temp, temp_humid_mutex, "TEMP", TEMPERATURE_HUMIDITY_SCREEN);
        process_sensor_data(sensor_data_buffer.humidity, &sensor_data_buffer.humid_reading_index, &sensor_data_buffer.average_humidity, temp_humid_mutex, "HUMID", TEMPERATURE_HUMIDITY_SCREEN);
        process_sensor_data(sensor_data_buffer.voc_measurement, &sensor_data_buffer.voc_reading_index, &sensor_data_buffer.average_voc, voc_mutex, "VOC", VOC_SCREEN);
       
        check_user_threshold();
        check_general_safety_value();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
