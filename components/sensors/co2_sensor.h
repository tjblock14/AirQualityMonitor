#ifndef CO2_SENSOR_H
#define CO2_SENSOR_H

#include "stdint.h"
#include "stdbool.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

void co2_task(void *parameter);
esp_err_t co2_read_data(uint16_t *raw_co2_concentration);
bool did_both_co2_sensors_read_valid(float co2_a, float co2_b);
void convert_co2_data_to_readable(uint16_t *raw_co2_concentration);

extern SemaphoreHandle_t co2_mutex;

#endif  // CO2_SENSOR_H