#ifndef USER_CONTROL_H
#define USER_CONTROL_H

#include "stdint.h"

void handle_button_press(int btn_id);
void user_button_task(void *parameter);

void set_backlight_updated();

#endif // USER_CONTROL_H