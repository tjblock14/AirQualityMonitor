#ifndef POWER_BUTTON_H
#define POWER_BUTTON_H

#include "stdbool.h"

void handle_pwr_btn_press();

void reset_buzzer_ack();
bool has_buzzer_been_acked();

#endif  // POWER_BUTTON_H