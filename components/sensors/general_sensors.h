
#ifndef SENSOR_TASKS_H
#define SENSOR_TASKS_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define I2C_TIMEOUT         10
#define MAX_SENSOR_READINGS 10

uint8_t crc_check(const uint8_t* data, uint16_t count);
bool is_buzzer_on();

typedef struct {
    uint16_t temperature[MAX_SENSOR_READINGS];
    uint16_t humidity[MAX_SENSOR_READINGS];
    uint16_t co2_concentration[MAX_SENSOR_READINGS];
    uint16_t voc_measurement[MAX_SENSOR_READINGS];
    uint8_t temp_reading_index;
    uint8_t humid_reading_index;
    uint8_t co2_reading_index;
    uint8_t voc_reading_index;
    uint16_t average_co2;
    uint16_t average_temp;
    uint16_t average_humidity;
    uint16_t  average_voc;
    uint16_t co2_user_threshold;
    uint16_t co2_generally_unsafe_value;
    uint16_t voc_user_threshold;
    uint16_t voc_generally_unsafe_value;
} sensor_readings_t;

void check_general_safety_value();
void check_user_threshold();
extern sensor_readings_t sensor_data_buffer;

#endif  //SENSOR_TASKS_H
