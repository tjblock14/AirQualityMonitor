set(srcs "esp_web_server.c" "wifi.c" "get_sensor_data.c")

idf_component_register(SRCS "${srcs}" INCLUDE_DIRS "."
                        REQUIRES 
                            esp_wifi
                            esp_http_server
                            nvs_flash
                            sensors)