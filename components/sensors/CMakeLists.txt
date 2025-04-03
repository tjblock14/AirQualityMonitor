set(srcs 
         "voc_sensor.c" 
         "co2_sensor.c"
         "temp_sensor.c"
         "general_sensors.c")

idf_component_register(SRCS "${srcs}" INCLUDE_DIRS "."
                        REQUIRES 
                            driver
                            gpio_setup
                            display
                            sensirion_files_voc)