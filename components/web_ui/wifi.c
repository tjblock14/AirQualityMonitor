#include "wifi.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "nvs_flash.h"

#define WIFI_SSID "TnT Dungeon"
#define WIFI_PASS "TayloafTbot1431"

static const char *TAG = "wifi_station";

// Wi-Fi Event Handler
static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        ESP_LOGI(TAG, "Connected to Wi-Fi");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        esp_netif_ip_info_t ip_info;
        esp_netif_t *esp_netif = esp_netif_get_handle_from_ifkey("WIFI_STA");
        esp_netif_get_ip_info(esp_netif, &ip_info);  // Corrected for v5.3.1
        ESP_LOGI(TAG, "Got IP Address: " IPSTR, IP2STR(&ip_info.ip));
    }
}

// Initialize Wi-Fi and connect to the network
void wifi_init_sta(void) {
    ESP_ERROR_CHECK(nvs_flash_init());  // Initialize NVS for storing Wi-Fi settings
    esp_netif_init();
    esp_event_loop_create_default();

    // Create the default Wi-Fi station interface
    esp_netif_t *esp_netif = esp_netif_create_default_wifi_sta();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    esp_wifi_start();
    esp_wifi_connect();
}
