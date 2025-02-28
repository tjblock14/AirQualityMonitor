
#ifndef SENSOR_TASKS_H
#define SENSOR_TASKS_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define I2C_TIMEOUT        10

//queue handles that the azure hub and display will read from
extern QueueHandle_t temp_data_queue, humid_data_queue, voc_data_queue, co2_data_queue;

uint8_t crc_check(const uint8_t* data, uint16_t count);

#endif  //SENSOR_TASKS_H
