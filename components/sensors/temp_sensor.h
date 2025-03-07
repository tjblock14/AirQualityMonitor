#ifndef TEMP_SENSOR_H
#define TEMP_SENSOR_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

extern QueueHandle_t temperature_data_queue;
extern QueueHandle_t humidity_data_queue;
extern SemaphoreHandle_t temp_humid_mutex;

void temp_humidity_task(void *parameter);

#endif