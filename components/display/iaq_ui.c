#include "i2c_config.h"
#include "iaq_ui.h"
#include "get_sensor_data.h"
#include "general_sensors.h"
#include "co2_sensor.h"
#include "voc_sensor.h"
#include "temp_sensor.h"
#include "ui_screen_inits.h"
#include "Userbuttons.h"
#include "user_control.h"

uint8_t clear_display_cmd[2] = {0x7C, 0x2D};


display_screen_pages_t get_next_screen_page(display_screen_pages_t current_page)
{
    display_screen_pages_t next_screen = STARTUP_SCREEN;
    switch(current_page)
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
            next_screen = BATTERY_LEVEL_SCREEN;
            break;
        case BATTERY_LEVEL_SCREEN:  // Go back to beginning, Will only enter settings screens if specified
            next_screen = TEMPERATURE_HUMIDITY_SCREEN;
            break;
        default:   // If there is some issue, default back to startup screen
            next_screen = STARTUP_SCREEN;
            break;
    }
    return next_screen;
}

/******************************
 * @brief This function will be called once on startup, and then every time the button to proceed to the next screen is pressed
 * @param current_page is the page that will be displayed on the screen when this function is called
 */
void set_ui_screen_page(display_screen_pages_t current_page)
{
    switch(current_page)
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
        case BATTERY_LEVEL_SCREEN:
            battery_screen_init();
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


/*******************
 * @brief The main task for the display. This will read from the data queues of the sensors and display it on the UI
 *******************/
void display_task(void *parameter)
{
    // Probably permanently remove since we go deep sleep but idk yet
    // i2c_master_transmit(i2c_display_device_handle, clear_display_cmd, sizeof(clear_display_cmd), pdMS_TO_TICKS(100));


    while(1)
    {

        // Get most recent average value from all sensors
        if(sensor_data_buffer.co2_reading_index >= MAX_SENSOR_READINGS)
        {
            if(xSemaphoreTake(co2_mutex, pdMS_TO_TICKS(20)) == pdTRUE)
            {
                sensor_data_buffer.average_co2 = get_average_sensor_data(sensor_data_buffer.co2_concentration, &sensor_data_buffer.co2_reading_index, "CO2");
                set_ui_screen_page(CO2_SCREEN);
                xSemaphoreGive(co2_mutex);
            }

        }
        
        // If this is true, both the temperature and humidity readings have reached 10, so average them both out
        if(sensor_data_buffer.temp_reading_index >= MAX_SENSOR_READINGS)
        {
            if(xSemaphoreTake(temp_humid_mutex, pdMS_TO_TICKS(20)) == pdTRUE)
            {
                sensor_data_buffer.average_temp = get_average_sensor_data(sensor_data_buffer.temperature, &sensor_data_buffer.temp_reading_index, "TEMP");
                sensor_data_buffer.average_humidity = get_average_sensor_data(sensor_data_buffer.humidity, &sensor_data_buffer.humid_reading_index, "HUMID");
                xSemaphoreGive(temp_humid_mutex);
            }
        }
        
        if(sensor_data_buffer.voc_reading_index >= MAX_SENSOR_READINGS)
        {
            if(xSemaphoreTake(voc_mutex, pdMS_TO_TICKS(50)) == pdTRUE)
            {
                sensor_data_buffer.average_voc = get_average_sensor_data(sensor_data_buffer.voc_measurement, &sensor_data_buffer.voc_reading_index, "VOC");
                xSemaphoreGive(voc_mutex);
            }
        }
        
        check_user_threshold();
        check_general_safety_value();

        // figuring this out to get it working before entering deep sleep
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
