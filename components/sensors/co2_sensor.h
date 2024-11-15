#ifndef CO2_SENSOR_H
#define CO2_SENSOR_H

#include "stdint.h"
#include "stdbool.h"
#include "esp_err.h"

esp_err_t co2_read_data(uint8_t sensor_address, uint16_t *raw_co2_concentration);
bool did_both_co2_sensors_read_valid(float co2_a, float co2_b);

#endif  // CO2_SENSOR_H