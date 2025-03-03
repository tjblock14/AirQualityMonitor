#ifndef VOC_SENSOR_H
#define VOC_SENSOR_H

#include "stdint.h"

void voc_task(void *parameter);
void voc_select_channel(uint8_t channel);

#endif  //VOC_SENSOR_H