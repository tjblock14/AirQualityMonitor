set(srcs "power_button.c")

idf_component_register(SRCS "${srcs}" INCLUDE_DIRS "."
                        REQUIRES 
                            driver
                            sensors
                            gpio_setup
                            display)
