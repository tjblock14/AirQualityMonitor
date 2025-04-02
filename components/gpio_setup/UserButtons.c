#include "driver/gpio.h"
#include "esp_timer.h"
#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/queue.h"
#include "Userbuttons.h"

#define DEBOUNCE_DELAY (30)
uint32_t last_button_press_time = 0;
QueueHandle_t user_button_queue = NULL;

const gpio_num_t USER_BUTTONS[] = {USR_BTN_ONE_PIN, USR_BTN_TWO_PIN, USR_BTN_THREE_PIN, USR_BTN_FOUR_PIN, PWR_BTN_PIN};
const size_t NUM_BUTTONS = sizeof(USER_BUTTONS) / sizeof(USER_BUTTONS[0]);

// Used to keep the device from entering deep sleep if a button has recently been pressed (user interaction)
bool recent_button_press = false;

/*******************************************
 * @brief This function will be called by the deep sleep task to check if the user has recently interacted with the device. This will prevent
 *        the device from entering deep sleep mode if there has been recent interaction
 *******************************************/
bool check_recent_user_interaction()
{

}

/*********************
 * @brief This function debounces button presses to ensure that when a button is pressed, it is only read once, 
 *        rather than a few times due to the signal ripple when a button is pressed
 ********************/
bool user_button_debounce()
{
    uint32_t current_time = esp_timer_get_time();
    if((current_time - last_button_press_time) > DEBOUNCE_DELAY)
    {
        last_button_press_time = current_time;
        return true; // Button successfully debounced, button was pressed
    }
    else
    {
        return false; // False button press
    }
}

/********************************
 * @brief This function is called when a button press is detected. It will debounce the button press, and then add
 *        the ID of the pressed button to the button queue
 */
static void IRAM_ATTR user_button_isr_handler(void* id)
{
  if(user_button_debounce())
  {
    // Add the ID of the button that was pressed to the queue
    int button_press_id = (int)id;
    xQueueSendFromISR(user_button_queue, &button_press_id, NULL);

    // Set recent button press variable to true
    recent_button_press = true;
  }  
}


/******************
 * @brief Configures the GPIO interrupts for all five buttons
 *****************/
void button_init()
{
    // Baisc configuration for interrupt, pin will be set before the ISR is initialized
    gpio_config_t btn_config = {
            .mode = GPIO_MODE_INPUT,
            .intr_type = GPIO_INTR_NEGEDGE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .pull_up_en = GPIO_PULLUP_DISABLE
    };

    ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1));

    // Initialize all five buttons as interrupts
    for(uint8_t i = 0; i < NUM_BUTTONS ; i++)
    {
        btn_config.pin_bit_mask = (1ULL << USER_BUTTONS[i]);
        ESP_ERROR_CHECK(gpio_config(&btn_config));
        ESP_ERROR_CHECK(gpio_isr_handler_add(USER_BUTTONS[i], user_button_isr_handler, (void*)USER_BUTTONS[i]));
    }  

    // Create the queue that will be read from for button presses
    user_button_queue = xQueueCreate(6, sizeof(USR_BTN_ONE_PIN));
}