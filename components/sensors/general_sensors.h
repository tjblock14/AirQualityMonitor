
#ifndef SENSOR_TASKS_H
#define SENSOR_TASKS_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define I2C_TIMEOUT         10
#define MAX_SENSOR_READINGS 10

uint8_t crc_check(const uint8_t* data, uint16_t count);

typedef struct {
    uint16_t temperature[MAX_SENSOR_READINGS];
    uint16_t humidity[MAX_SENSOR_READINGS];
    uint16_t co2_concentration[MAX_SENSOR_READINGS];
    int32_t voc_measurement[MAX_SENSOR_READINGS];
    uint8_t temp_reading_index;
    uint8_t humid_reading_index;
    uint8_t co2_reading_index;
    uint8_t voc_reading_index;
    uint16_t average_co2;
    uint16_t average_temp;
    uint16_t average_humidity;
    int32_t average_voc;
} sensor_readings_t;

extern sensor_readings_t sensor_data_buffer;

#endif  //SENSOR_TASKS_H
