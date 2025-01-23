#ifndef GET_SENSOR_DATA_H
#define GET_SENSOR_DATA_H

#include "stdint.h"

void web_ui_get_data_task(void *parameter);
uint16_t get_avg_co2_value(void);

#endif //GET_SENSOR_DATA_H