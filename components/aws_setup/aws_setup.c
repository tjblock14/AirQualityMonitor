#include "aws_setup.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "mqtt_client.h"
#include "esp_err.h"
#include "esp_log.h"
#include "mqtt.h"

//primary connection string for device in azure IoT hub
static const char *TAG = "AWS";

void mqtt_publish_dummy_data(const char *topic, const char *message)
{
    esp_err_t err = esp_mqtt_client_publish(esp_mqtt_client, topic, message, 0, 1, 0);
    if (err >= 0) {
        ESP_LOGI("MQTT", "Message published to topic %s: %s", topic, message);
    } else {
        ESP_LOGE("MQTT", "Failed to publish message: %s", esp_err_to_name(err));
    }
}


void aws_task()
{
    // Wait for MQTT connection
    vTaskDelay(pdMS_TO_TICKS(5000));  // Delay to ensure connection is established

    const char *dummy_message = "{\"temperature\": 25.5, \"humidity\": 60.2}";

    while (1) 
    {
        mqtt_publish_dummy_data("air-quality-monitor/temperature", dummy_message);
        vTaskDelay(pdMS_TO_TICKS(10000));  // Publish every 10 seconds
    }
}