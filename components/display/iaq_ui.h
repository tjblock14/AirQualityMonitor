#ifndef IAQ_UI_H
#define IAQ_UI_H

#include "stdint.h"
#include "stdbool.h"

extern uint8_t clear_display_cmd[2];

void display_task(void *parameter);

typedef enum {
    STARTUP_SCREEN = 0,
    TEMPERATURE_HUMIDITY_SCREEN,
    CO2_SCREEN,
    VOC_SCREEN,

    // Settings Screens
    SET_CO2_THRESH_SCREEN,
    SET_VOC_THRESH_SCREEN,
    ERROR_SCREEN
} display_screen_pages_t;

display_screen_pages_t get_next_screen_page(display_screen_pages_t displayed_page);
void set_ui_screen_page(display_screen_pages_t setpage);
bool is_initial_data_ready();

void power_display_on();
void power_down_display();

extern display_screen_pages_t current_page;

#endif // IAQ_UI_H