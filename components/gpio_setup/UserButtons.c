#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_sleep.h"
#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/queue.h"
#include "Userbuttons.h"
#include "esp_log.h"

#define DEBOUNCE_DELAY (30000)   // 30ms in us
#define AVOID_SLEEP_TIME (120000000)  // 2 minutes in us
#define TEN_SECOND_HOLD (10000000)   // 10 seconds in us 
uint32_t last_button_press_time = AVOID_SLEEP_TIME;   // Initializing this to avoid sleep time could help prevent an issue with always avoiding deep sleep for 2 minutes on wakeup
uint8_t last_button_pressed_id = 0;
QueueHandle_t user_button_queue = NULL;

const gpio_num_t USER_BUTTONS[] = {USR_BTN_ONE_PIN, USR_BTN_TWO_PIN, USR_BTN_THREE_PIN, USR_BTN_FOUR_PIN, PWR_BTN_PIN};
const size_t NUM_BUTTONS = sizeof(USER_BUTTONS) / sizeof(USER_BUTTONS[0]);

/******************************************
 * @brief Checks if the button that is pressed was held for 10 seconds. Only needed for power button
 * @returns true if held for 10 seconds, false if not
 *****************************************/
bool was_button_held_for_ten_seconds(int btn_id)
{
    uint32_t press_time = 0;
    uint32_t release_time = 0;
    uint32_t press_duration = 0;

    press_time = esp_timer_get_time();
    while((gpio_get_level(btn_id) == 0) && (esp_timer_get_time() - press_time) <= TEN_SECOND_HOLD)
    {}  // empty while loop, This will wait until either the button was held for 10 seconds, or the button was released before then

    release_time = esp_timer_get_time();
    press_duration = release_time - press_time;
    if(press_duration >= TEN_SECOND_HOLD)
    {
        return true;
    }
    else // not held for 10 seconds
    {
        return false;
    }
}

/**********************************************
 * @brief Check if button was pressed and held for three seconds, used for power button to force enter sleep mode
 ***********************************************/
bool was_button_held_for_three_seconds()
{
    return false;
}

/*******************************************
 * @brief This function will be called by the deep sleep task to check if the user has recently interacted with the device. This will prevent
 *        the device from entering deep sleep mode if there has been recent interaction
 *******************************************/
bool check_recent_user_interaction()
{
    // Might want to check which button was pressed too, because if pwr button held for 3 seconds, enter sleep
    // If two minutes have passed since last user interaction, set flag to let deep sleep task enter deep sleep
    // Maybe need to work on this function
    if(last_button_press_time >= AVOID_SLEEP_TIME)
    {
       return false;
    }
    else
    {
        return true; // Will prevent deep sleep
    }
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
ESP_EARLY_LOGW("BTN", "ISR TRIGGERED");
  if(user_button_debounce())
  {
    // Add the ID of the button that was pressed to the queue
    int button_press_id = (int)id;
    xQueueSendFromISR(user_button_queue, &button_press_id, NULL);

    // Set recent button press variable to true and set id of last button press
   // recent_button_press = true;
    last_button_pressed_id = button_press_id;
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
    user_button_queue = xQueueCreate(6, sizeof(int));
}