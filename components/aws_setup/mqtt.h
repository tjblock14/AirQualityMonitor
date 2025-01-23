#ifndef MQTT_H
#define MQTT_H

#include "mqtt_client.h"

extern esp_mqtt_client_handle_t esp_mqtt_client;

void mqtt_init();

#endif //MQTT_H