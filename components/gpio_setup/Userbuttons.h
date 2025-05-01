#ifndef USERBUTTONS_H
#define USERBUTTONS_H

#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/queue.h"
#include "stdbool.h"

#define PWR_BTN_PIN       14
#define USR_BTN_ONE_PIN   9
#define USR_BTN_TWO_PIN   4
#define USR_BTN_THREE_PIN 11
#define USR_BTN_FOUR_PIN  13
#define LARGE_BUZZER_PIN  3

extern QueueHandle_t user_button_queue;
void button_init();
bool was_button_held_for_ten_seconds(int btn_id);
bool check_recent_user_interaction();
bool user_button_debounce(int btn_id);

// Used for only turning the display off once in consistent deep sleep
bool is_display_off_in_consistent_sleep();
void set_display_off_in_sleep();
void reset_display_off_in_sleep();

#endif // USERBUTTONS_H