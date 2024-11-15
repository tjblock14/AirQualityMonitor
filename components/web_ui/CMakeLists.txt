set(srcs "esp_web_server.c" "wifi.c")

idf_component_register(SRCS "${srcs}" INCLUDE_DIRS "."
                        REQUIRES 
                            esp_wifi
                            esp_http_server
                            nvs_flash)