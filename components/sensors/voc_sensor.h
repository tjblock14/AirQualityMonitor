#ifndef VOC_SENSOR_H
#define VOC_SENSOR_H

#include "stdint.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

extern SemaphoreHandle_t voc_mutex;

void voc_task(void *parameter);

#endif  //VOC_SENSOR_H