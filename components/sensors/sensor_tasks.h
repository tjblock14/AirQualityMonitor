
#ifndef SENSOR_TASKS_H
#define SENSOR_TASKS_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#define I2C_TIMEOUT        10
#define VOC_MUX_ADDR       0x70

//queue handles that the azure hub and display will read from
extern QueueHandle_t temp_data_queue, humid_data_queue, voc_data_queue, co2_data_queue;
extern SemaphoreHandle_t co2_mutex;

void temp_humidity_task(void *parameter);
void voc_task(void *parameter);
void co2_task(void *parameter);
esp_err_t crc_check(const uint8_t* data, uint16_t count, uint8_t checksum);

#endif  //SENSOR_TASKS_H
