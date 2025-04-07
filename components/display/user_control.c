#include "user_control.h"
#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/queue.h"
#include "esp_log.h"
#include "Userbuttons.h"
#include "iaq_ui.h"
#include "ui_screen_inits.h"
#include "power_button.h"

// User cannot set threshold greater than this, as this is already dangerous
#define MAX_CO2_THRESHOLD 1000
#define MIN_CO2_THRESHOLD 600
// VOC levels should ideally be below 400 ppb
#define MAX_VOC_THRESH    400
#define MIN_VOC_THRESH    200

//Default levels, based on guidlines from World Health Organization
uint16_t co2_threshold = 1000;
uint16_t voc_threshold = 400;

/***************************
 * @brief If user button 2 is pressed while the screen is either the CO2 or VOC level,
 *        it will take the user to the screen where they can use button 3 and 4 to
 *        increment/decrement the setpoint for their liking. They then press button 2 again 
 *        to return. If the button is pressed on any other page, nothing will happen
 *************************/
void get_setpoint_screen()
{
    display_screen_pages_t next_page = ERROR_SCREEN;
    switch(current_page)
    {
        case CO2_SCREEN:   // User wants to change setpoint, go to set threshold screen
            next_page = SET_CO2_THRESH_SCREEN;
            break;
        case VOC_SCREEN:
            next_page = SET_VOC_THRESH_SCREEN;
            break;
        case SET_CO2_THRESH_SCREEN: // Done setting threshold, return to CO2 screen
            next_page = CO2_SCREEN;
            break;
        case SET_VOC_THRESH_SCREEN: 
            next_page = VOC_SCREEN;
            break;
        default:  // do nothing
            next_page = current_page;
            break;
    }
    // Update screen with the next page
    set_ui_screen_page(next_page);
}

/********************************
 * @brief when the button is pressed while on one of the setpoint screens, increment the
 *        setpoint. If max value is reached or the current screen is not a setpoint screen, do nothing
 *******************************/
void increment_gas_setpoint()
{
    if(current_page == SET_CO2_THRESH_SCREEN)
    {
        if(co2_threshold < MAX_CO2_THRESHOLD)
        {
            co2_threshold++;
        }
    }
    //will have to add in functions for display for both of these
    else if(current_page == SET_VOC_THRESH_SCREEN)
    {
        if(voc_threshold < MAX_VOC_THRESH)
        {
            voc_threshold++;
        }
    }
    else{} // Do nothing
}

/********************************
 * @brief when the button is pressed while on one of the setpoint screens, decrement the
 *        setpoint. If max value is reached or the current screen is not a setpoint screen, do nothing
 *******************************/
void decrement_gas_setpoint()
{
    if(current_page == SET_CO2_THRESH_SCREEN)
    {
        if(co2_threshold > MIN_CO2_THRESHOLD)
        {
            co2_threshold--;
        }
    }
    else if(current_page == SET_VOC_THRESH_SCREEN)
    {
        if(voc_threshold > MIN_CO2_THRESHOLD)
        {
            voc_threshold--;
        }
    }
    else{} // Do nothing
}

/**********************************
 * @brief This funciton will read the ID of the pressed button from the queue, and then determine
 *        proceed with necessary steps, specific to the button
 **********************************/
void handle_button_press(uint8_t btn_id)
{
    //will probably want to do the queue receive in the main loop of display task,
    // and if button is received, then call this funciton and send button id as parameter
    
        switch(btn_id)
        {
            case USR_BTN_ONE_PIN:  // Increment the screen page, initialize display with new screen
                current_page = get_next_screen_page(current_page);
                set_ui_screen_page(current_page);
                break;
            case USR_BTN_TWO_PIN:  // Go to setpoint screen
                get_setpoint_screen();
                break;
            case USR_BTN_THREE_PIN:
                increment_gas_setpoint();
                break;
            case USR_BTN_FOUR_PIN:
                decrement_gas_setpoint();
                break;
            case PWR_BTN_PIN:
                handle_pwr_btn_press();
                break;
            default:
                break;
        }
}

void user_button_task(void *parameter)
{
    while(1)
    {
        // Check for button presses, task blocked if nothing in queue
        uint8_t button_pressed_id = 0;
        if(xQueueReceive(user_button_queue, &button_pressed_id, portMAX_DELAY))
        {
            ESP_LOGI("BTN", "Button press sensed at GPIO %d", button_pressed_id);
             handle_button_press(button_pressed_id);
        }
    }
}