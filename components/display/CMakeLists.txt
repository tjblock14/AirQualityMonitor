set(srcs 
        "iaq_ui.c"
        "get_sensor_data.c"
        "ui_screen_inits.c"
        "user_control.c")

idf_component_register(SRCS "${srcs}" INCLUDE_DIRS "."
                        REQUIRES 
                            driver
                            sensors
                            gpio_setup
                            button_specifics)
