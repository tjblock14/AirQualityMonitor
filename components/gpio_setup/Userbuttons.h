#ifndef USERBUTTONS_H
#define USERBUTTONS_H

#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/queue.h"
#include "stdbool.h"

#define PWR_BTN_PIN       14
#define USR_BTN_ONE_PIN   10
#define USR_BTN_TWO_PIN   11
#define USR_BTN_THREE_PIN 12
#define USR_BTN_FOUR_PIN  13

extern QueueHandle_t user_button_queue;
void button_init();

#endif // USERBUTTONS_H