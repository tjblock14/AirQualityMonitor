#include "i2c_config.h"
#include "iaq_ui.h"
#include "get_sensor_data.h"
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

    // Overridden to CO2 screen for testing
    current_page = CO2_SCREEN;
    i2c_master_transmit(i2c_display_device_handle, clear_display_cmd, sizeof(clear_display_cmd), pdMS_TO_TICKS(100));

    while(1)
    {
        set_ui_screen_page(current_page);

        // Update value to put on screen, depending on the screen



        // Depending on the current screen and if not in sleep mode, update screen with newest data
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
