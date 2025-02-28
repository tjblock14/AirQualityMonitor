#ifndef USER_CONTROL_H
#define USER_CONTROL_H

#include "stdint.h"

void handle_button_press(uint8_t btn_id);
void user_button_task(void *parameter);

extern uint16_t co2_threshold;
extern uint16_t voc_threshold;

#endif // USER_CONTROL_H