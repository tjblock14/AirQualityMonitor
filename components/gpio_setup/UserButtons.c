#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_sleep.h"
#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/queue.h"
#include "Userbuttons.h"
#include "esp_log.h"

#define GPIO_INPUT_PIN_SEL  ((1ULL << USR_BTN_ONE_PIN) | (1ULL << USR_BTN_TWO_PIN) | (1ULL << USR_BTN_THREE_PIN) | (1ULL << USR_BTN_FOUR_PIN) | (1ULL << PWR_BTN_PIN))
#define DEBOUNCE_DELAY (pdMS_TO_TICKS(20))   //10 ms delay
#define AVOID_SLEEP_TIME (120000000)  // 2 minutes in us
#define TEN_SECOND_HOLD (10000000)   // 10 seconds in us 
uint32_t last_button_press_time = AVOID_SLEEP_TIME;  // Make sure we do not avoid sleep if no user interaction since wake
uint8_t last_button_pressed_id = 0;
QueueHandle_t user_button_queue = NULL;

const int USER_BUTTONS[] = {USR_BTN_ONE_PIN, USR_BTN_TWO_PIN, USR_BTN_THREE_PIN, USR_BTN_FOUR_PIN, PWR_BTN_PIN};
const size_t NUM_BUTTONS = sizeof(USER_BUTTONS) / sizeof(USER_BUTTONS[0]);

// This variable ensures that once the device is consistently in sleep mode with no user interaction, to only turn the backlights off once
RTC_DATA_ATTR bool display_turned_off_in_sleep = false;
bool wakeup_reason_checked = false;

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

// This function is called upon user interaction so that once it goes back to sleep, this variable will be reset
void reset_display_off_in_sleep()
{
    display_turned_off_in_sleep = false;
}

// This function is used when the display is being powered down so that it only needs to be powered down once, not repeatedly
void set_display_off_in_sleep()
{
    display_turned_off_in_sleep = true;
}

// This funciton is used to check whether or not the display has already been turned off
bool is_display_off_in_consistent_sleep()
{
    return display_turned_off_in_sleep;
}

void get_wakeup_reason_first_time()
{
    esp_sleep_wakeup_cause_t wakeup_cause = esp_sleep_get_wakeup_cause();
    if(wakeup_cause == ESP_SLEEP_WAKEUP_EXT0)
    {
        last_button_press_time = esp_timer_get_time();
        reset_display_off_in_sleep();
    }
    else if(wakeup_cause == ESP_SLEEP_WAKEUP_UNDEFINED)
    {
        last_button_press_time = 0;
    }
}


/*******************************************
 * @brief This function will be called by the deep sleep task to check if the user has recently interacted with the device. This will prevent
 *        the device from entering deep sleep mode if there has been recent interaction
 *******************************************/
bool check_recent_user_interaction()
{
    // If the power button was pressed to wake the device from deep sleep, make sure it avoids deep sleep
    // only check this the first time the function is called to ensure deep sleep is not constantly avoided
    if(!wakeup_reason_checked)
    {
        get_wakeup_reason_first_time();
        wakeup_reason_checked = true;
    }
    // Get the current run time. If it is more than 2 minutes pst the last time that a button as pressed, return false.
    // If not, rturn true, preventing deep sleep will be avoided
    uint32_t current_time = esp_timer_get_time();
    if((current_time - last_button_press_time) >= AVOID_SLEEP_TIME)
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
bool user_button_debounce(int btn_id)
{
    // Disable the interrupt so only the first press is detected as a press
    gpio_intr_disable(btn_id);

    bool button_status[8] = {0};

    // Check the button status every 10ms for 80ms total
    for(uint8_t i = 0; i < 8; i++)
    {
        button_status[i] = gpio_get_level(btn_id);
        vTaskDelay(DEBOUNCE_DELAY); // delay 10ms then re-read
    }

    // If there are fours consecutive lows in this check, then the button was pressed
    for(uint8_t j = 0; j < 4; j++)
    {
        if (!button_status[j] && !button_status[j+1] && !button_status[j+2] && !button_status[j+3])
        {
            // Found 4 consecutive lows, take down last button press time, re-enable interrupt, and return true for button press
            last_button_press_time = esp_timer_get_time();
            gpio_intr_enable(btn_id);
            return true;
        }
    }
    // Return false if the end of the function is reached, meaning there were not four consecutive lows, and re-enable interrupt
    gpio_intr_enable(btn_id);
    return false;
}

/********************************
 * @brief This function is called when a button press is detected. It will debounce the button press, and then add
 *        the ID of the pressed button to the button queue
 */
static void IRAM_ATTR user_button_isr_handler(void* id)
{
    int button_press_id = (int)id;
    // Add the ID of the button that was pressed to the queue
    xQueueSendFromISR(user_button_queue, &button_press_id, NULL);
}


gpio_config_t lrg_bzr_pin_config = {
    .mode = GPIO_MODE_OUTPUT,
    .intr_type = GPIO_MODE_OUTPUT,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .pull_up_en   = GPIO_PULLUP_DISABLE,
    .pin_bit_mask = (1ULL << LARGE_BUZZER_PIN)
};

    // Basic configuration for interrupt, pin will be set before the ISR is initialized
    gpio_config_t btn_config = {
        .mode = GPIO_MODE_INPUT,
        .intr_type = GPIO_INTR_NEGEDGE,
        .pin_bit_mask = GPIO_INPUT_PIN_SEL,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE
};

/******************
 * @brief Configures the GPIO interrupts for all five buttons
 *****************/
void button_init()
{
	gpio_config(&lrg_bzr_pin_config);

    ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1));

    // Initialize all five buttons as interrupts
    ESP_ERROR_CHECK(gpio_config(&btn_config));
    for(uint8_t i = 0; i < NUM_BUTTONS ; i++)
    {
        ESP_ERROR_CHECK(gpio_isr_handler_add(USER_BUTTONS[i], user_button_isr_handler, (void*)USER_BUTTONS[i]));
    }  

    // Create the queue that will be read from for button presses
    user_button_queue = xQueueCreate(6, sizeof(USR_BTN_ONE_PIN));
}