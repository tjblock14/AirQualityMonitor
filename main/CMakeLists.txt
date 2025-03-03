idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES 
                        gpio_setup
                        sensors
                        web_ui
                        aws_setup
                        display)