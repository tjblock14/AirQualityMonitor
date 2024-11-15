
#ifndef SENSOR_TASKS_H
#define SENSOR_TASKS_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define I2C_TIMEOUT        10
#define VOC_MUX_ADDR       0x70

//queue handles that the azure hub and display will read from
extern QueueHandle_t temp_readings_queue, humid_readings_queue, voc_readings_queue, co2_readings_queue;

void temp_humidity_task(void *parameter);
void voc_task(void *parameter);
void co2_task(void *parameter);
esp_err_t crc_check(const uint8_t* data, uint16_t count, uint8_t checksum);

#endif  //SENSOR_TASKS_H
