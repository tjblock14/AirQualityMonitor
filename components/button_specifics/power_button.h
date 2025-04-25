#ifndef POWER_BUTTON_H
#define POWER_BUTTON_H

#include "stdbool.h"

void handle_pwr_btn_press();

void reset_user_buzzer_ack();
bool has_user_buzzer_been_acked();
bool has_safety_buzzer_been_acked();
void reset_safety_buzzer_ack();

#endif  // POWER_BUTTON_H