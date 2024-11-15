set(srcs "sensor_tasks.c" 
         "voc_sensor.c" 
         "co2_sensor.c")

idf_component_register(SRCS "${srcs}" INCLUDE_DIRS "."
                        REQUIRES 
                            driver
                            gpio_setup)