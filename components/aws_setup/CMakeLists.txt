set(srcs "mqtt.c" "aws_setup.c")

idf_component_register(SRCS "${srcs}" INCLUDE_DIRS "."
                        REQUIRES 
                            sensors
                            esp_wifi
                            mqtt
                            certifications)