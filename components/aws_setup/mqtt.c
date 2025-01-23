#include "aws_setup.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "mqtt_client.h"
#include "esp_err.h"
#include "esp_log.h"
#include "mqtt.h"
#include "device_certification.h"   //certificate from Amazon AWS IoT thing
#include "private_key.h"            //private key created by Amazon AWS
#include "amazon_root_ca_cert.h"    //Amazon root CA certificate

#define AWS_IOT_ENDPOINT       "a29amwa12734pw-ats.iot.us-east-2.amazonaws.com"  // AWS IoT Endpoint
#define MQTT_BROKER_URI        "mqtts://" AWS_IOT_ENDPOINT
#define CLIENT_ID              "air-quality-monitor"
#define THING_NAME             "air-quality-monitor"

static const char *TAG = "MQTT";
esp_mqtt_client_handle_t esp_mqtt_client;

esp_mqtt_client_config_t mqtt_config = {
    .broker = {
        .address.uri = MQTT_BROKER_URI,
        .verification.certificate = (const char *)AmazonRootCA1_pem,
        .verification.certificate_len = AmazonRootCA1_pem_len
    },
    .credentials = {
        .client_id = CLIENT_ID,
        .authentication =
        {
            .certificate = (const char *)device_certification_security_pem_crt,
            .certificate_len = device_certification_security_pem_crt_len,
            .key = (const char *)privatekey_pem_key,
            .key_len = privatekey_pem_key_len,
        }
    }
};

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{

    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch(event->event_id)
    {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Event - Connected");
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "Event - Disconnected. Attempting to reconnect");
            //esp_mqtt_client_reconnect(esp_mqtt_client);
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "Event - Subscribed");
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "Event - Unsubscribed");
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "Event - Published");
            break;
        default:
            ESP_LOGI(TAG, "Other event - %d", event->event_id);
            break;
    }
}

/**
 * @brief This function initializes the MQTT protocol so that the esp32 device can 
 * communicate with the AWS Iot hub
 */
void mqtt_init()
{
    esp_mqtt_client = esp_mqtt_client_init(&mqtt_config);

    // Ensure that there was a mqtt client initialized
    if (esp_mqtt_client == NULL)
    {
        ESP_LOGE(TAG, "Error initializing MQTT Client");
        return;
    }

    // Listen for all types of MQTT events and then proceed in the handler function
    esp_mqtt_client_register_event(esp_mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

    //start the mqtt client
    esp_mqtt_client_start(esp_mqtt_client);
}