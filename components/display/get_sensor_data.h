#ifndef GET_SENSOR_DATA_H
#define GET_SENSOR_DATA_H

#include "stdint.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

uint16_t get_co2_level_for_display();
uint16_t get_average_sensor_data(void *sensor_data, uint8_t *sensor_index, const char *sensor_name);


#endif //GET_SENSOR_DATA_H