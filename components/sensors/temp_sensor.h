#ifndef TEMP_SENSOR_H
#define TEMP_SENSOR_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

extern SemaphoreHandle_t temp_humid_mutex;
extern QueueHandle_t     temp_humid_voc_queue;

void temp_humidity_task(void *parameter);

#endif