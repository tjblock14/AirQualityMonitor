set(srcs 
        "UserButtons.c" 
        "i2c_config.c" )

idf_component_register(SRCS "${srcs}" INCLUDE_DIRS "."
                        REQUIRES 
                            driver
                            esp_timer)